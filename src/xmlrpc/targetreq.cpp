/*
 * Target display and manipulation.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#include "xmlrpcd.h"
#include "targetreq.h"
#include "altplot.h"
#include "dirsupport.h"
#include "imgpreview.h"
#include "xmlrpc++/urlencoding.h"
#include "../db/simbad/simbadtarget.h"
#include "../utils/rts2command.h"

#ifdef HAVE_PGSQL
#include "../utilsdb/observationset.h"
#include "../utilsdb/imageset.h"
#include "../utilsdb/targetset.h"
#endif /* HAVE_PGSQL */

#include "../utils/radecparser.h"

using namespace XmlRpc;
using namespace rts2xmlrpc;

#ifdef HAVE_PGSQL

void Targets::authorizedExecute (std::string path, HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));

	if (vals.size () == 0)
	{
		listTargets (params, response_type, response, response_length);
	}
	else
	{
		if (vals[0] == "form")
		{
			processForm (params, response_type, response, response_length);
			return;
		}
		if (vals[0] == "api")
		{
			processAPI (params, response_type, response, response_length);
			return;
		}
		// get target id..
		char *endptr;
		int tar_id = strtol (vals[0].c_str (), &endptr, 10);
		if (*endptr != '\0')
			throw rts2core::Error ("Expected target number!");
		if (tar_id < 0)
			throw rts2core::Error ("Target id < 0");

		Target *tar = createTarget (tar_id, Rts2Config::instance ()->getObserver ());
		if (tar == NULL)
			throw rts2core::Error ("Cannot find target with given ID");

		switch (vals.size ())
		{
			case 1:
				printTarget (tar, response_type, response, response_length);
				break;
			case 2:
				if (vals[1] == "api")
				{
					callAPI (tar, params, response_type, response, response_length);
					break;
				}
				if (vals[1] == "main")
				{
					printTarget (tar, response_type, response, response_length);
					break;
				}
				if (vals[1] == "info")
				{
					printTargetInfo (tar, response_type, response, response_length);
					break;
				}
				if (vals[1] == "images")
				{
					printTargetImages (tar, params, response_type, response, response_length);
					break;
				}
				if (vals[1] == "obs")
				{
					printTargetObservations (tar, response_type, response, response_length);
					break;
				}
#ifdef HAVE_LIBJPEG
				if (vals[1] == "altplot")
				{
					plotTarget (tar, params, response_type, response, response_length);
					break;
				}
#endif /* HAVE_LIBJPEG */
			default:
				throw rts2core::Error ("Invalid path!");
		}
	}
}

void Targets::listTargets (XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;
	printHeader (_os, "List of targets", NULL, NULL, "refreshTargets();");

	includeJavaScript (_os, "equ.js");

	_os << "<script type='text/javascript'>\n"
		"var targets = [];\n"
		"var sortcol = 0;\n"
		"var asc = true;\n"
		"function sort_table(a, b) {\n"
			"if (asc) { return a[sortcol] - b[sortcol]; }\n"
			"else { return b[sortcol] - a[sortcol]; }\n"
		"}\n"

		"function fillTD(td,data,header,i){\n"
			"switch(header.t){\n"
				"case 'r':\n"
					"td.innerHTML = (new HMS(data)).toString();\n"
					"break;\n"
				"case 'd':\n"
					"td.innerHTML = (new DMS(data)).toString();\n"
					"break;\n"
				"case 'Alt':\n"
					"td.id = 'alt_' + i;\n"
					"break;"
				"case 'Az':\n"
					"td.id = 'az_' + i;\n"
					"break;"
				"default:\n"
					"td.innerHTML = data;\n"
			"}\n"
		"}\n"
		"function fillTR(tr,t,i,h){\n"
			"for (var j in t){\n"
				"var td = tr.insertCell(-1);\n"
				"fillTD(td,t[j],h[j],i);\n"
			"}\n"
		"}\n"
		"function fillTable(tab){\n"
			"targets.sort(sort_table);\n"
			"for (var i in targets) {\n"
				"tr = tab.insertRow(-1);\n"
				"fillTR (tr,targets[i],i,t.h);\n"
			"}\n"
		"}\n"
		"function resortTable(tabid,col){\n"
			"sortcol = col;\n"
			"var e = document.getElementById(tabid);\n"
			"var l = e.rows.length;\n"
			"for (var i = 1; i < l; i++){\n"
				"e.deleteRow(1);}\n"
			"fillTable(e);\n"
			"updateTable();\n"
		"}\n"
		"function refreshTargets(){\n"
			"var hr = new XMLHttpRequest();\n"
			"hr.open('GET','api/',true);\n"
			"hr.onreadystatechange = function() {\n"
				"if (hr.readyState == 4 && hr.status == 200) {\n"
					"t = JSON.parse(hr.responseText);\n"
					"targets = t.d;\n"
					"var tab = document.createElement('table');\n"
					"tab.id = 'tartab';\n"
					// write header
					"var th = tab.createTHead();\n"
					"var tr = th.insertRow(0);\n"
					"for (var h in t.h) {\n"
						"var td = tr.insertCell(-1);\n"
						"td.innerHTML = t.h[h].n;\n"
						"td.setAttribute('onclick', 'resortTable(\"tartab\",' + h + '); return true;');\n"
					"}\n"
					"fillTable(tab);\n"
					"var e = document.getElementById('targets');\n"
					"e.innerHTML = '';\n"
					"e.appendChild(tab);\n"
					"updateTable();\n"
					"setTimeout('updateTable()',2000);\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
		"}\n"

		"function updateTable() {\n"
			"var jd = ln_get_julian_from_sys();\n"
			"var st = ln_get_mean_sidereal_time(jd);\n"
			"hAz = new DMS();\n"
			"hAlt = new DMS();\n"
			"observer = new LngLat(" << Rts2Config::instance ()->getObserver ()->lng << "," << Rts2Config::instance ()->getObserver ()->lat << ")\n;"
			"for (var i in targets) {\n"
				"var t = targets[i];\n"
				"var radec = new RaDec(targets[i][2],targets[i][3], observer);\n"
				"var altaz = radec.altaz();\n"
				"t[4] = altaz.alt;\n"
				"t[5] = altaz.az;\n"
				"ln_deg_to_dms(altaz.alt,hAlt);\n"
				"ln_deg_to_dms(altaz.az,hAz);\n"
				"document.getElementById('alt_' + i).innerHTML = hAlt.toString ();\n"
				"document.getElementById('az_' + i).innerHTML = hAz.toString ();\n"
			"}\n"
		"}\n"

		"</script>\n";

	_os << "<form action='form' method='post'><div id='targets'>Loading...</div>\n";

#ifdef HAVE_LIBJPEG
	_os << "<input type='submit' value='Plot' name='plot'/>";
#endif // HAVE_LIBJPEG
	_os << "</form>";
	
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::processForm (XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
#ifdef HAVE_LIBJPEG
	if (!strcmp (params->getString ("plot", "xxx"), "Plot"))
	{
		response_type = "image/jpeg";

		AltPlot ap (params->getInteger ("w", 800), params->getInteger ("h", 600));
		Magick::Geometry size (params->getInteger ("w", 800), params->getInteger ("h", 600));
	
		double from = params->getDouble ("from", 0);
		double to = params->getDouble ("to", 0);
	
		if (from < 0 && to == 0)
		{
			// just fr specified - from
			to = time (NULL);
			from += to;
		}
		else if (from == 0 && to == 0)
		{
			// default - one hour
			to = time (NULL);
			from = to - 86400;
		}


		std::list < int > ti;

		for (XmlRpc::HttpParams::iterator iter = params->begin (); iter != params->end (); iter++)
		{
			if (!strcmp (iter->getName (), "tarid"))
				ti.push_back (atoi (iter->getValue ()));
		}

		rts2db::TargetSet ts = rts2db::TargetSet ();
		ts.load (ti);
	
		Magick::Image mimage (size, "white");
		ap.getPlot (from, to, &ts, &mimage);
	
		Magick::Blob blob;
		mimage.write (&blob, "jpeg");
	
		response_length = blob.length();
		response = new char[response_length];
		memcpy (response, blob.data(), response_length);
		return;
	}
#endif // HAVE_LIBJPEG
}

void Targets::processAPI (XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "application/json";
	std::ostringstream _os;

	const char *t = params->getString ("t", NULL);

	double JD = params->getDouble ("jd", ln_get_julian_from_sys ());

	rts2db::TargetSet ts;

	if (t != NULL)
		ts = rts2db::TargetSet (t);
	else
		ts = rts2db::TargetSet ();
	ts.load ();

	_os << "{\"h\":["
		"{\"n\":\"ID\",\"t\":\"n\"},"
		"{\"n\":\"Target name\",\"t\":\"s\"},"
		"{\"n\":\"RA\",\"t\":\"r\"},"
		"{\"n\":\"DEC\",\"t\":\"d\"},"
		"{\"n\":\"Alt\",\"t\":\"Alt\"},"
		"{\"n\":\"Az\",\"t\":\"Az\"}],"
		"\"d\" : [";

	for (rts2db::TargetSet::iterator iter = ts.begin (); iter != ts.end (); iter++)
	{
		struct ln_equ_posn pos;
		struct ln_hrz_posn hrz;
		iter->second->getPosition (&pos, JD);
		iter->second->getAltAz (&hrz, JD);
		if (iter != ts.begin ())
			_os << ",";
		_os << "[" << iter->second->getTargetID () << ",\"" << iter->second->getTargetName () << "\",";
		if (isnan (pos.ra) || isnan (pos.dec) || isnan (hrz.alt) || isnan (hrz.az))
			_os << "0,0,0,0]";
		else
			_os << pos.ra << "," << pos.dec << "," << hrz.alt << "," << hrz.az << "]";
	}

	_os << "] }";

	returnJSON (_os, response_type, response, response_length);
}

void Targets::printTargetHeader (int tar_id, std::ostringstream &_os)
{
	std::ostringstream prefix;
	prefix << ((XmlRpcd *)getMasterApp ())->getPagePrefix () << "/targets/" << tar_id << "/";
	_os << "<p><a href='" << prefix.str ()
		<< "main/'>main page</a>&nbsp;<a href='" << prefix.str ()
		<< "info/'>info page</a>&nbsp;<a href='" << prefix.str ()
		<< "images/'>images</a>&nbsp;<a href='" << prefix.str () 
		<< "obs/'>observations</a>&nbsp;<a href='" << prefix.str ()
		<< "altplot/'>altitude plot</a></p>";
}

void Targets::callAPI (Target *tar, HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	if (!canExecute ())
		throw XmlRpcException ("you do not have permission to change anything");
	const char *e = params->getString ("e", NULL);
	if (e != NULL)
	{
		tar->setTargetEnabled (!strcmp (e, "true"), true);
		tar->save (true);
		returnJSON ("1", response_type, response, response_length);
		return;
	}
	e = params->getString ("slew", NULL);
	if (e != NULL)
	{
		XmlRpcd *master = (XmlRpcd *) getMasterApp ();
		connections_t::iterator iter = master->getConnections ()->begin ();
		master->getOpenConnectionType (DEVICE_TYPE_MOUNT, iter);
		if (iter == master->getConnections ()->end ())
		  	throw XmlRpcException ("Telescope is not connected");
		struct ln_equ_posn pos;
		tar->getPosition (&pos);
		(*iter)->queCommand (new rts2core::Rts2CommandMove (master, NULL, pos.ra, pos.dec));

		// return status..
		Rts2ValueRaDec *telRaDec = (Rts2ValueRaDec *) ((*iter)->getValue ("TEL"));
		Rts2ValueAltAz *telAltAz = (Rts2ValueAltAz *) ((*iter)->getValue ("TEL_"));
		Rts2ValueRaDec *tarRaDec = (Rts2ValueRaDec *) ((*iter)->getValue ("TAR"));

		std::ostringstream os;
		os << "{\"remaining\":" << ((*iter)->getValueDouble ("move_end") - ((XmlRpcd *)getMasterApp ())->getNow ())
			<< ", \"tel\": {\"ra\":" << telRaDec->getRa () << ", \"dec\":" << telRaDec->getDec ()
			<< ", \"alt\":" << telAltAz->getAlt () << ", \"az\":" << telAltAz->getAz ()
			<< "}, \"tar\": {\"ra\":" << tarRaDec->getRa () << ", \"dec\":" << tarRaDec->getDec ()
			<< "} }";
		returnJSON (os.str ().c_str (), response_type, response, response_length);
		return;
	}
	e = params->getString ("next", NULL);
	if (e != NULL)
	{
		XmlRpcd *master = (XmlRpcd *) getMasterApp ();
		connections_t::iterator iter = master->getConnections ()->begin ();
		master->getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iter);
		if (iter == master->getConnections ()->end ())
		  	throw XmlRpcException ("Executor is not running");
		(*iter)->queCommand (new rts2core::Rts2CommandExecNext (master, tar->getTargetID ()));

		std::ostringstream os;
		os << "{\"status\": 0 }";
		returnJSON (os.str ().c_str (), response_type, response, response_length);
		return;
	}
	e = params->getString ("now", NULL);
	if (e != NULL)
	{
		XmlRpcd *master = (XmlRpcd *) getMasterApp ();
		connections_t::iterator iter = master->getConnections ()->begin ();
		master->getOpenConnectionType (DEVICE_TYPE_EXECUTOR, iter);
		if (iter == master->getConnections ()->end ())
		  	throw XmlRpcException ("Executor is not running");
		(*iter)->queCommand (new rts2core::Rts2CommandExecNow (master, tar->getTargetID ()));

		std::ostringstream os;
		os << "{\"status\": 0 }";
		returnJSON (os.str ().c_str (), response_type, response, response_length);
		return;
	}
	throw XmlRpcException ("invalid API request");
}

void Targets::printTarget (Target *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	printHeader (_os, (std::string ("Target ") + tar->getTargetName ()).c_str (), NULL, NULL, "altAzTimer ();");

	includeJavaScript (_os, "equ.js");

	struct ln_equ_posn tradec;

	tar->getPosition (&tradec);

	_os << "<script type='text/javascript'>\n"
	"radec = new RaDec(" << tradec.ra << "," << tradec.dec << ", new LngLat(" << Rts2Config::instance ()->getObserver ()->lng << "," << Rts2Config::instance ()->getObserver ()->lat << "));\n"
	"hAlt = new DMS();\n"
	"hAz = new DMS();\n"
	"hSt = new HMS();\n"

	"function altAzTimer() {\n"
		"altaz = radec.altaz ();\n"
		"ln_deg_to_dms(altaz.alt,hAlt);\n"
		"ln_deg_to_dms(altaz.az,hAz);\n"
		"var jd = ln_get_julian_from_sys();\n"
		"var st = ln_get_mean_sidereal_time(jd);\n"
		"ln_deg_to_hms(st,hSt);\n"
//ln_get_mean_sidereal_time(ln_get_julian_from_sys()),hSt);\n"
		"document.getElementById('altaz').innerHTML = hAlt.toString() + ' ' + hAz.toString();\n"
		"document.getElementById('st').innerHTML = hSt.toString() + ' ' + (2/4) + ' ' + st;\n"
		"document.getElementById('jd').innerHTML = jd;\n"
		"setTimeout('altAzTimer()',100);\n"
	"}\n";

	if (canExecute ())
	{
		//enable target
		_os << "function tar_enable (enable) {\n"
			"var status;\n"
			"var hr = new XMLHttpRequest();\n"
			"hr.open('GET','api?e=' + enable);\n"
			"hr.onreadystatechange = function () {\n"
				"if (hr.readyState == 4 && hr.status == 200 ) {\n"
					"status = JSON.parse(hr.responseText);\n"
					"document.getElementById('tar_enable').checked = enable;\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
			"return false;\n"
		"}\n"

		"function tar_slew () {\n"
			"var status = {};\n"
			"var hr = new XMLHttpRequest();\n"
			"document.getElementById('slew').innerHTML = '<div id=\"slew_message\">requesting slew..</div><div><span id=\"slew_tar_ra\"></span>&nbsp;<span id=\"slew_tar_dec\"></span></div><div><span id=\"slew_tel_ra\"></span>&nbsp;<span id=\"slew_tel_dec\"></span></div><div><span id=\"slew_tel_alt\"></span>&nbsp;<span id=\"slew_tel_az\"></span></div>';\n"
			"document.getElementById('slew').style.display = '';\n"
			"hr.open('GET','api?slew=yes');\n"
			"hr.onreadystatechange = function () {\n"
				"if (hr.readyState == 4 && hr.status == 200 ) {\n"
					"status = JSON.parse(hr.responseText);\n"
					"var d = new DMS();\n"
					"var h = new HMS();\n"
					"document.getElementById('slew_message').innerHTML = 'slewing ' + status.remaining;\n"
					"ln_deg_to_hms(status.tar.ra,h);\n"
					"ln_deg_to_dms(status.tar.dec,d);\n"
					"document.getElementById('slew_tar_ra').innerHTML = h.toString ();\n"
					"document.getElementById('slew_tar_dec').innerHTML = d.toString ();\n"
					"ln_deg_to_hms(status.tel.ra,h);\n"
					"ln_deg_to_dms(status.tel.dec,d);\n"
					"document.getElementById('slew_tel_ra').innerHTML = h.toString ();\n"
					"document.getElementById('slew_tel_dec').innerHTML = d.toString ();\n"
					"ln_deg_to_hms(status.tel.az,d);\n"
					"document.getElementById('slew_tel_az').innerHTML = d.toString ();\n"
					"ln_deg_to_dms(status.tel.alt,d);\n"
					"document.getElementById('slew_tel_alt').innerHTML = d.toString ();\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
		"}\n"

		"function tar_next () {\n"
			"var status = {};\n"
			"var hr = new XMLHttpRequest();\n"
			"document.getElementById('next').innerHTML = '<div id=\"next_message\">requesting next..</div>';\n"
			"document.getElementById('next').style.display = '';\n"
			"hr.open('GET','api?next=yes');\n"
			"hr.onreadystatechange = function () {\n"
				"if (hr.readyState == 4 && hr.status == 200 ) {\n"
					"status = JSON.parse(hr.responseText);\n"
					"document.getElementById('next_message').innerHTML = 'queued';\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
		"}\n"

		"function tar_now () {\n"
			"var status = {};\n"
			"var hr = new XMLHttpRequest();\n"
			"document.getElementById('now').innerHTML = '<div id=\"now_message\">requesting now..</div>';\n"
			"document.getElementById('now').style.display = '';\n"
			"hr.open('GET','api?now=yes');\n"
			"hr.onreadystatechange = function () {\n"
				"if (hr.readyState == 4 && hr.status == 200 ) {\n"
					"status = JSON.parse(hr.responseText);\n"
					"document.getElementById('now_message').innerHTML = 'executed';\n"
				"}\n"
			"}\n"
			"hr.send(null);\n"
		"}\n";

	}

	_os << "</script>\n";

	printTargetHeader (tar->getTargetID (), _os);

	// javascript to send requests..

	
	_os << "<p><div>Name " << tar->getTargetName () << "</div>"
		"<div>RA DEC " << LibnovaRaDec (&tradec) << "</div>"
		"<div>ALT AZ <span id='altaz'/></div>"
		"<div>Sidereal Time " << ln_get_mean_sidereal_time(ln_get_julian_from_sys()) << "<span id='st'/></div>"
		"<div>JD " << ln_get_julian_from_sys () << " <span id='jd'/></div>";
	if (canExecute ())
	{
		_os << "<div><button type='button' onclick='tar_slew();'>slew to target</a></div>"
		"<div id='slew' style='display:none'>not slewing</div>"
		"<div><button type='button' onclick='tar_next();'>next target</a></div>"
		"<div id='next' style='display:none'>nothing</div>"
		"<div><button type='button' onclick='tar_now();'>now (immediate) target</a></div>"
		"<div id='now' style='display:none'>nothing</div>"
		"<div>Enabled: <input type='checkbox' id='tar_enable' onclick='tar_enable (this.checked);' checked='"
		<< (tar->getTargetEnabled () ? "yes" : "no")
		<< "'/></div>";
	}
	else
	{
	  	_os << "<div>Enabled: " << (tar->getTargetEnabled () ? "yes" : "no") << "</div>";
	}

	_os << "</p>";

	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetInfo (Target *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;
	
	printHeader (_os, (std::string ("Target ") + tar->getTargetName ()).c_str ());

	printTargetHeader (tar->getTargetID (), _os);

	_os << "<pre>";

	double JD = ln_get_julian_from_sys ();
	Rts2InfoValOStream ivos (&_os);
	tar->sendInfo (ivos, JD);

	_os << "</pre>";
	
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetImages (Target *tar, HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	int pageno = params->getInteger ("p", 1);
	int pagesiz = params->getInteger ("s", 40);

	if (pageno <= 0)
		pageno = 1;

	int istart = (pageno - 1) * pagesiz;
	int ie = istart + pagesiz;
	int in = 0;

	int prevsize = params->getInteger ("ps", 128);
	const char * label = params->getString ("lb", "%Y-%m-%d %H:%M:%S @OBJECT");
	std::string lb (label);
	XmlRpc::urlencode (lb);
	const char * label_encoded = lb.c_str ();

	Previewer preview = Previewer ();

	printHeader (_os, (std::string ("Images of target ") + tar->getTargetName ()).c_str (), preview.style ());

	printTargetHeader (tar->getTargetID (), _os);
	
	preview.script (_os, label_encoded);
		
	_os << "<p>";

	preview.form (_os, pageno, prevsize, pagesiz, label);
	
	_os << "</p><p>";

	rts2db::ImageSetTarget is = rts2db::ImageSetTarget (tar->getTargetID ());
	is.load ();

	if (is.size () > 0)
	{
		_os << "<p>";

		for (rts2db::ImageSetTarget::iterator iter = is.begin (); iter != is.end (); iter++)
		{
			in++;
			if (in <= istart)
				continue;
			if (in > ie)
				break;
			preview.imageHref (_os, in, (*iter)->getAbsoluteFileName (), prevsize, label_encoded);
		}

		_os << "</p>";
	}
	else
	{
		_os << "<p>There isn't any image for target " << tar->getTargetName ();
	}

	_os << "</p><p>Page ";
	int i;
	
	for (i = 1; i <= ((int) is.size ()) / pagesiz; i++)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno);
	if (in % pagesiz)
	 	preview.pageLink (_os, i, pagesiz, prevsize, label_encoded, i == pageno);
	_os << "</p>";
	
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void Targets::printTargetObservations (Target *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	printHeader (_os, (std::string ("Observations of target ") + tar->getTargetName ()).c_str ());
	printTargetHeader (tar->getTargetID (), _os);

	rts2db::ObservationSet os = rts2db::ObservationSet ();
	os.loadTarget (tar->getTargetID ());

	if (os.size () > 0)
	{
		_os << "<table>";

		for (rts2db::ObservationSet::iterator iter = os.begin (); iter != os.end (); iter++)
		{
			_os << "<tr><td>" << iter->getObsId () << "</td><td>" << iter->getTargetName () << "</td></tr>";
		}

		_os << "</table>";
	}
	else
	{
		_os << "<p>There isn't any observation for target " << tar->getTargetName ();
	}

	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#ifdef HAVE_LIBJPEG

void Targets::plotTarget (Target *tar, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	response_type = "image/jpeg";

	AltPlot ap (params->getInteger ("w", 800), params->getInteger ("h", 600));
	Magick::Geometry size (params->getInteger ("w", 800), params->getInteger ("h", 600));

	double from = params->getDouble ("from", 0);
	double to = params->getDouble ("to", 0);

	if (from < 0 && to == 0)
	{
		// just fr specified - from
		to = time (NULL);
		from += to;
	}
	else if (from == 0 && to == 0)
	{
		// default - one hour
		to = time (NULL);
		from = to - 86400;
	}

	Magick::Image mimage (size, "white");
	ap.getPlot (from, to, tar, &mimage);

	Magick::Blob blob;
	mimage.write (&blob, "jpeg");

	response_length = blob.length();
	response = new char[response_length];
	memcpy (response, blob.data(), response_length);
}

#endif /* HAVE_LIBJPEG */

void AddTarget::authorizedExecute (std::string path, XmlRpc::HttpParams *params, const char* &response_type, char* &response, size_t &response_length)
{
	// get path and possibly date range
	std::vector <std::string> vals = SplitStr (path, std::string ("/"));

	switch (vals.size ())
	{
		case 0:
			askForTarget (response_type, response, response_length);
			return;
		case 1:
			if (vals[0] == "confirm")
			{
				confimTarget (params->getString ("target", ""), response_type, response, response_length);
				return;
			}
			if (vals[0] == "new_target")
			{
				newTarget (params->getString ("oriname", ""), params->getString ("name", ""), params->getInteger ("tarid", INT_MAX), params->getDouble ("ra", rts2_nan ("f")), params->getDouble ("dec", rts2_nan ("f")), response_type, response, response_length);
				return;
			}
			if (vals[0] == "schedule")
			{
				schedule  (params->getInteger ("tarid", INT_MAX), response_type, response, response_length);
				return;
			}
	}

	throw rts2core::Error ("Unknown action for addtarget");
}

void AddTarget::askForTarget (const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	printHeader (_os, "Add new target");

	_os << "<p>Please provide anything which can be used to identify what you would like to observer. Valid inputs are:<ul>"
		"<li>RA DEC in various formats. DEC must be divided with + or - sign. Those are valid inputs<ul><li>300 +24</li> <li>10:20 +33:12</li> <li>10:20:30 -12:34:59</li></ul></li>"
		"<li>SIMBAD resolvable name</li>"
		"<li>any target from RTS2 target database</li>"
		"<li>any Minor planet name resolvable by MPEC</li>"
		"<li>one line MPEC</li>"
		"</ul></p><form name='add_target' action='confirm' method='get'><input type='text' textwidth='20' name='target'/><input type='submit' value='Add'/></form>";
	
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::confimTarget (const char *tar, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	if (strlen (tar) == 0)
	{
		throw XmlRpc::XmlRpcException ("Invalid target - valid targer name must contain at least one character.");
	}
	else
	{
		printHeader (_os, "Confirm target");

		struct ln_equ_posn pos;

		pos.ra = pos.dec = rts2_nan ("f");

		// check if its among already known targets
		rts2db::TargetSetByName ts_n = rts2db::TargetSetByName (tar);
		ts_n.load ();

		_os << "<p>You entered " << tar << "</p>";
		if (ts_n.size () > 0)
		{
			_os << "<p>Following targets with name similar to which you provide were found. Please click on link to select one for scheduling, or continue bellow to create a new one.<ul>";

			for (rts2db::TargetSetByName::iterator iter = ts_n.begin (); iter != ts_n.end (); iter++)
			{
				// print target together with option to create schedule from this one..
				iter->second->getPosition (&pos);
				_os << "<li><a href='schedule?tarid=" << iter->second->getTargetID () << "'>" << iter->second->getTargetName () << "</a> " << LibnovaRa (pos.ra) << " " << LibnovaDec (pos.dec) << "</li>";
			}
			_os << "</ul></p>";
		}

		// check if when we parse it by ra dec, we will get something..
		if (parseRaDec (tar, pos.ra, pos.dec) == 0)
		{
			// then look for targets close to this one - within 1 deg
			rts2db::TargetSet ts_r = rts2db::TargetSet (&pos, 1);
			ts_r.load ();
			if (ts_r.size () > 0)
			{
				_os << "<p>Following targets were found within one degree from given position. Please click on link to select one for scheduling, or continue bellow entering a new target.<ul>";
				for (rts2db::TargetSetByName::iterator iter = ts_r.begin (); iter != ts_r.end (); iter++)
				{
					// print target together with option to create schedule from this one..
					iter->second->getPosition (&pos);
					_os << "<li><a href='schedule?tarid=" << iter->second->getTargetID () << "'>" << iter->second->getTargetName () << "</a> " << LibnovaRa (pos.ra) << " " << LibnovaDec (pos.dec) << "</li>";
				}
				_os << "</ul></p>";
			}
		}
		// check for simbad
		Target *target = new rts2db::SimbadTarget (tar);
		if (target->load () == 0)
		{
			target->getPosition (&pos);
		}
		delete target;

		// print new target
		if (! (isnan (pos.ra) || isnan (pos.dec)))
		{
			_os << "<p>If you would like to enter new target with RA DEC " << LibnovaRaDec (pos.ra, pos.dec)
				<< ", then fill fields bellow and click submit: <form name='new_target' action='new_target' method='get'><input type='hidden' name='ra' value='"
				<< pos.ra << "'/><input type='hidden' name='dec' value='" << pos.dec 
				<< "'><input type='hidden' name='oriname' value='" << tar << "'/>Target name:<input type='text' name='name' value='"
				<< tar << "'/><br/>Optional target ID - left blank for autogenerated<input type='text' name='tarid' value=' '/><br/><input type='submit' value='Create target'/></form></p>";
		}

		printFooter (_os);
	}

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::newTarget (const char *oriname, const char *name, int tarid, double ra, double dec, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	ConstTarget *constTarget = new ConstTarget ();
	constTarget->setPosition (ra, dec);
	constTarget->setTargetName (name);
	constTarget->setTargetType (TYPE_OPORTUNITY);

	int ret;

	if (tarid != INT_MAX)
		ret = constTarget->save (false, tarid);
	else
		ret = ((Target *) constTarget)->save (false);
	
	if (ret)
		throw XmlRpcException ("Target with given ID already exists");

	printHeader (_os, "Create new target");
	
	_os << "<p>Target with name " << name << " and ID " << constTarget->getTargetID ()
		<< " sucessfully created. Please click <a href='schedule?tarid=" << constTarget->getTargetID ()
		<< "'>here</a> to continue with scheduling the target.</p>";
		
	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

void AddTarget::schedule (int tarid, const char* &response_type, char* &response, size_t &response_length)
{
	std::ostringstream _os;

	Target *tar = createTarget (tarid, Rts2Config::instance ()->getObserver ());

	if (tar == NULL)
		throw XmlRpcException ("Cannot find target with given ID!");

	printHeader (_os, (std::string ("Scheduling target ") + tar->getTargetName ()).c_str ());
	
	_os << "<p>Please choose how your observations should be scheduled and peformed:<form name=''";

	_os << "</p>";

	printFooter (_os);

	response_type = "text/html";
	response_length = _os.str ().length ();
	response = new char[response_length];
	memcpy (response, _os.str ().c_str (), response_length);
}

#endif /* HAVE_PGSQL */
