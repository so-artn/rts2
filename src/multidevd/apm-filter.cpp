/**
 * APM filter wheel (ESA TestBed telescope)
 * Copyright (C) 2014 - 2015 Stanislav Vitek <standa@vitkovi.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "apm-filter.h"

using namespace rts2filterd;

APMFilter::APMFilter (const char *name, rts2core::ConnAPM *conn):Filterd (0, NULL, name), APMMultidev (conn)
{
	filterNum = 0;
	filterSleep = 3;

	addOption ('e', NULL, 1, "IP and port (separated by :)");
//	addOption ('s', "filter_sleep", 1, "how long wait for filter change");
}

int APMFilter::getFilterNum ()
{
	sendUDPMessage ("F999");

	return filterNum;
}

int APMFilter::setFilterNum (int new_filter)
{
	char *m = (char *)malloc(5*sizeof(char));
	sprintf (m, "F00%d", new_filter);

	sendUDPMessage (m);

	filterNum = new_filter;

	sleep (filterSleep);

	return Filterd::setFilterNum (new_filter);
}

int APMFilter::homeFilter ()
{
	setFilterNum (0);
	
	return 0;
}

/**
int APMFilter::initHardware ()
{
	sendUDPMessage ("F999");

	return 0;
}*/

int APMFilter::sendUDPMessage (const char * _message)
{
        char * response = (char *)malloc (20*sizeof (char));

	if (getDebug())
		logStream (MESSAGE_DEBUG) << "command: " << _message << sendLog;

	int n = apmConn->sendReceive (_message, response, 20);
	response[n] = '\0';

	if (getDebug())
		logStream (MESSAGE_DEBUG) << "reponse: " << response << sendLog;

	if (n > 4)
	{
		if (response[0] == 'F')
		{
			// echo from controller "Echo: [cmd]"
			return 10;
		}
	}
	else if (n == 4)
	{	
		filterNum = response[3];
	}

	return 0;
}
