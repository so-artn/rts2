/* 
 * Sensor reading clarity cloud sensor log file.
 * Copyright (C) 2014 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sensord.h"
#include "connnotify.h"

namespace rts2sensord
{

/**
 * Clarity cloud sensor.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class Clarity:public SensorWeather
{
	public:
		Clarity (int argc, char **argv);

		virtual void fileModified (struct inotify_event *event);

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual bool isGoodWeather ();

	private:
		rts2core::ValueFloat *tempSky;

		rts2core::ValueFloat *tempOutside1;
		rts2core::ValueFloat *tempOutside2;

		// parse one line from the file
		int parseLine (const char *line);

		rts2core::ConnNotify *notifyConn;

		const char *clarityFile;
};

}

using namespace rts2sensord;

Clarity::Clarity (int argc, char **argv):SensorWeather (argc, argv)
{
	createValue (tempSky, "temp_sky", "sky temparature", false);
	createValue (tempOutside1, "temp_outside_1", "outside temperature #1", false);
	createValue (tempOutside2, "temp_outside_2", "outside temparature #2", false);

	addOption ('f', NULL, 1, "clarity 1 cloud sensor log file");

	clarityFile = NULL;

	notifyConn = new rts2core::ConnNotify (this);
	addConnection(notifyConn);
}

void Clarity::fileModified (struct inotify_event *event)
{
#define CLARITY_LOG_BUFFER_SIZE   140
	if (event->len > 0 && strcmp (event->name, clarityFile) == 0)
	{
		int f = open (clarityFile, O_RDONLY);
		struct stat fs;
		int ret = fstat (f, &fs);
		if (ret)
			return;
		char buf[CLARITY_LOG_BUFFER_SIZE];
		if (fs.st_size > CLARITY_LOG_BUFFER_SIZE)
		{
			off_t off = lseek (f, -CLARITY_LOG_BUFFER_SIZE, SEEK_END);
			if (off == -1)
				return;
		}
		ret = read (f, buf, CLARITY_LOG_BUFFER_SIZE);
		if (ret <= 0)
			return;
		// find end and before end '\n'
		char *end_cl = rindex (buf, '\n');
		if (end_cl == NULL)
			return;
		*end_cl = '\0';
		end_cl = rindex (buf, '\n');
		if (end_cl == NULL)
			return;
		parseLine (end_cl);
	}
}

int Clarity::processOption (int opt)
{
	switch (opt)
	{
		case 'f':
			clarityFile = optarg;
			break;
		default:
			return SensorWeather::processOption (opt);
	}
	return 0;
}

int Clarity::initHardware ()
{
	if (clarityFile == NULL)
	{
		std::cerr << "you must specify path to the clarity file" << std::endl;
		return -1;
	}

	notifyConn->addWatch (clarityFile);

	return 0;
}

int Clarity::parseLine (const char *line)
{
	struct tm lasttime;
	wchar_t c;
	float sky, out1, out2;
	int i1, i2, i3, i4, i5, i6;
	int ret = sscanf (line, "%d-%d-%d %d:%d:%d %C %f %f %f %d %d %d %d.%d %d", &lasttime.tm_year, &lasttime.tm_mon, &lasttime.tm_mday, &lasttime.tm_hour, &lasttime.tm_min, &lasttime.tm_sec, &c, &sky, &out1, &out2, &i1, &i2, &i3, &i4, &i5, &i6);
	if (ret != 16)
	{
		logStream (MESSAGE_ERROR) << "cannot parse line " << line << sendLog;
		return -1;
	}

	tempSky->setValueFloat (sky);
	tempOutside1->setValueFloat (out1);
	tempOutside2->setValueFloat (out2);

	setInfoTime (mktime (&lasttime));

	return 0;
}

bool Clarity::isGoodWeather ()
{
	return SensorWeather::isGoodWeather ();
}

int main (int argc, char **argv)
{
	Clarity device (argc, argv);
	return device.run ();
}
