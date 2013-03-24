/* 
 * Big Brother server connector.
 * Copyright (C) 2010 Petr Kubánek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "bbserver.h"
#include "xmlrpc++/urlencoding.h"
#include "xmlrpcd.h"

#include "rts2db/observation.h"

#include "hoststring.h"
#include "daemon.h"

#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

using namespace rts2xmlrpc;

// thread routine
void *updateBB (void *arg)
{
	BBServer *bbserver = (BBServer *) arg;

	char *conn_name;
	asprintf (&conn_name, "connection_%d", bbserver->getObservatoryId ());

	((rts2db::DeviceDb *) getMasterApp ())->initDB (conn_name);

	while (true)
	{
		int rid = bbserver->requests.pop (true);
		switch (rid)
		{
			case -1:
				bbserver->sendUpdate ();
				break;
			default:
				bbserver->sendObservationUpdate (rid);
				break;
		}
	}
	return NULL;
}

void BBServer::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_XMLRPC_BB:
			queueUpdate (-1);
			server->addTimer (getCadency (), event);
			return;
	}
	rts2core::Object::postEvent (event);
}

void BBServer::sendUpdate ()
{
	if (client == NULL)
	{
		client = new XmlRpcClient (serverApi.c_str (), &_uri);
		if (password.length ())
		{
			std::ostringstream auth;
			auth << observatoryId << ":" << password;
			client->setAuthorization (auth.str ().c_str ());
		}
	}

	std::ostringstream body;

	bool first = true;

	body << "{";

	for (rts2core::connections_t::iterator iter = server->getConnections ()->begin (); iter != server->getConnections ()->end (); iter++)
	{
		if ((*iter)->getName ()[0] == '\0')
			continue;
		if (first)
			first = false;
		else
			body << ",";
		body << "\"" << (*iter)->getName () << "\":{";
		rts2json::sendConnectionValues (body, *iter, NULL, NAN, true);
		body << "}";
	}

	body << "}";

	char * reply;
	int reply_length;

	std::ostringstream url;
	if (_uri)
		url << _uri;
	url << "/api/observatory?observatory_id=" << observatoryId;

	int ret = client->executePostRequest (url.str ().c_str (), body.str ().c_str (), reply, reply_length);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "Error requesting " << serverApi.c_str () << sendLog;
		delete client;
		client = NULL;
		return;
	}

	JsonParser *result = json_parser_new ();

	GError *error = NULL;
	json_parser_load_from_data (result, reply, reply_length, &error);
	if (error)
	{
		logStream (MESSAGE_ERROR) << "unable to parse JSON " << reply << sendLog;
		g_error_free (error);
		g_object_unref (result);
		delete[] reply;
		return;
	}

	server->bbSend (json_object_get_double_member (json_node_get_object (json_parser_get_root (result)), "localtime"));

	g_error_free (error);
	g_object_unref (result);
	delete[] reply;
}

void addNonNan (std::ostringstream &os, double val, const char *vname)
{
	if (!isnan (val))
		os << "&" << vname << "=" << val;
}

void BBServer::sendObservationUpdate (int observationId)
{
	rts2db::Observation obs(observationId);
	if (obs.load ())
		return;
	// find and load associated plan
	rts2db::Plan plan(obs.getPlanId ());
	if (plan.load ())
		return;
	
	std::ostringstream url;
	url.setf (std::ios_base::fixed, std::ios_base::floatfield);
	if (_uri)
		url << _uri;
	url << "/api/observation?observatory_id=" << observatoryId << "&obs_id=" << obs.getObsId () << "&obs_tar_id=" << obs.getTargetId ();

	addNonNan (url, obs.getObsRa (), "obs_ra");
	addNonNan (url, obs.getObsDec (), "obs_dec");
	addNonNan (url, obs.getObsSlew (), "obs_slew");
	addNonNan (url, obs.getObsStart (), "obs_start");
	addNonNan (url, obs.getObsEnd (), "obs_end");

	char * reply;
	int reply_length;

	int ret = client->executePostRequest (url.str ().c_str (), NULL, reply, reply_length);
	if (!ret)
	{
		logStream (MESSAGE_ERROR) << "Error requesting " << serverApi.c_str () << sendLog;
		delete client;
		client = NULL;
		return;
	}
}

void BBServer::queueUpdate (int reqId)
{
	if (send_thread == 0)
	{
		pthread_create (&send_thread, NULL, updateBB, (void *) this);
	}
	requests.push (reqId);
}

void BBServers::sendUpdate ()
{
	for (BBServers::iterator iter = begin (); iter != end (); iter++)
		iter->queueUpdate (-1);
}

void BBServers::sendObservatoryUpdate (int observatoryId, int request)
{
	for (BBServers::iterator iter = begin (); iter != end (); iter++)
	{
		if (iter->isObservatory (observatoryId))
		{
			iter->queueUpdate (request);
			return;
		}
	}
}

size_t BBServers::queueSize ()
{
	size_t ret = 0;
	for (BBServers::iterator iter = begin (); iter != end (); iter++)
		ret += iter->queueSize ();
	
	return ret;
}
