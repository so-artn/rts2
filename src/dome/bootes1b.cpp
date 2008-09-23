/* 
 * Dome driver for Bootes 1 B observatory.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "ford.h"

#define ROOF_TIMEOUT  120		 // in seconds

typedef enum
{
	PORT_1,
	PORT_2,
	PORT_3,
	PORT_4,
	DOMESWITCH,
	PORT_6,
	TEL_SWITCH,
	PORT_8,
	// A
	OPEN_END_1,
	CLOSE_END_1,
	CLOSE_END_2,
	OPEN_END_2,
	// 0xy0,
	PORT_13,
	RAIN_SENSOR
} outputs;

using namespace rts2dome;

namespace rts2dome
{

/**
 * Control for Bootes1 dome.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Bootes1B:public Ford
{
	private:
		time_t timeOpenClose;
		bool domeFailed;

		bool isMoving ();

		int lastWeatherCheckState;
	
		Rts2ValueInteger *sw_state;
		Rts2ValueTime *ignoreRainSensorTime;

	protected:
		virtual bool isGoodWeather ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();
		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();

	public:
		Bootes1B (int argc, char **argv);
		virtual ~ Bootes1B (void);
		virtual int init ();

		virtual int info ();

		virtual int commandAuthorized (Rts2Conn * conn);
};

}

Bootes1B::Bootes1B (int argc, char **argv)
:Ford (argc, argv)
{
	createValue (sw_state, "sw_state", "switch state", false, RTS2_DT_HEX);

	createValue (ignoreRainSensorTime, "ignore_rain_time", "time when rain sensor will be ignored", false);
	ignoreRainSensorTime->setValueDouble (nan ("f"));

	lastWeatherCheckState = -1;

	timeOpenClose = 0;
	domeFailed = false;
}


Bootes1B::~Bootes1B (void)
{
}


bool
Bootes1B::isGoodWeather ()
{
	if (getIgnoreMeteo ())
		return true;
	int ret = zjisti_stav_portu ();
	if (ret)
		return false;

	// rain sensor, ignore if dome was recently opened
	if (getPortState (RAIN_SENSOR))
	{
		int weatherCheckState = (getPortState (OPEN_END_2) << 3)
			| (getPortState (CLOSE_END_2) << 2)
			| (getPortState (CLOSE_END_1) << 1)
			| (getPortState (OPEN_END_1));
		if (lastWeatherCheckState == -1)
			lastWeatherCheckState = weatherCheckState;

		// all switches must be off and previous reading must show at least one was on, know prerequsity for failed rain sensor
		if (weatherCheckState != lastWeatherCheckState || (getState () & DOME_OPENING) || (getState () & DOME_CLOSING))
		{
			logStream (MESSAGE_WARNING) << "ignoring faulty rain sensor" << sendLog;
			ignoreRainSensorTime->setValueDouble (getNow () + 120);
		}
		else
		{
			// if we are not in ignoreRainSensorTime period..
			if (isnan (ignoreRainSensorTime->getValueDouble ()) || getNow () > ignoreRainSensorTime->getValueDouble ())
			{
				lastWeatherCheckState = weatherCheckState;
				setWeatherTimeout (3600);
	  			return false;
			}
		}
		lastWeatherCheckState = weatherCheckState;
	}
	else
	{
		lastWeatherCheckState = (getPortState (OPEN_END_2) << 3)
			| (getPortState (CLOSE_END_2) << 2)
			| (getPortState (CLOSE_END_1) << 1)
			| (getPortState (OPEN_END_1));
	}

	return Ford::isGoodWeather ();
}


int
Bootes1B::init ()
{
	int ret;
	ret = Ford::init ();
	if (ret)
		return ret;

	ret = zjisti_stav_portu ();
	if (ret)
		return -1;
	// set dome state..
	if (getPortState (CLOSE_END_1) && getPortState (CLOSE_END_2))
	{
		setState (DOME_CLOSED, "Init state is closed");
	} else if (getPortState (OPEN_END_1) && getPortState (OPEN_END_2))
	{
		setState (DOME_OPENED, "Init state is opened");
	} else
	{
		// not opened, not closed..
		return -1;
	}

	// switch on telescope
	ZAP (TEL_SWITCH);

	return 0;
}


int
Bootes1B::info ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (ret)
		return -1;

	sw_state->setValueInteger ((getPortState (CLOSE_END_1) << 3)
		| (getPortState (CLOSE_END_2) << 2)
		| (getPortState (OPEN_END_1) << 1)
		| getPortState (OPEN_END_2));

	return Ford::info ();
}


bool
Bootes1B::isMoving ()
{
	int ret;
	ret = zjisti_stav_portu ();
	if (getPortState (CLOSE_END_1) == getPortState (CLOSE_END_2)
		&& getPortState (OPEN_END_1) == getPortState (OPEN_END_2)
		&& (getPortState (CLOSE_END_1) != getPortState (OPEN_END_1)))
		return false;
	return true;
}


int
Bootes1B::startOpen ()
{
	if (getState () & DOME_OPENING)
	{
		if (isMoving ())
			return 0;
		return -1;
	}
	if ((getState () & DOME_OPENED)
		&& (getPortState (OPEN_END_1) || getPortState (OPEN_END_2)))
	{
		return 0;
	}
	if (getState () & DOME_CLOSING)
		return -1;

	ZAP(DOMESWITCH);
	sleep (1);
	VYP(DOMESWITCH);
	sleep (1);

	time (&timeOpenClose);
	timeOpenClose += ROOF_TIMEOUT;

	return 0;
}


long
Bootes1B::isOpened ()
{
	time_t now;
	time (&now);
	// timeout
	if (timeOpenClose > 0 && now > timeOpenClose)
	{
		logStream (MESSAGE_ERROR) << "Bootes1B::isOpened timeout" <<
			sendLog;
		domeFailed = true;
		maskState (DOME_DOME_MASK, DOME_CLOSED, "dome opened with errror");
		domeOpenStart ();
		return USEC_SEC;
	}
	if (isMoving ())
		return USEC_SEC;
	if (getPortState (OPEN_END_1) || getPortState (OPEN_END_2))
		return -2;
	if (getPortState (CLOSE_END_1) && getPortState (CLOSE_END_2))
		return USEC_SEC;
	logStream (MESSAGE_ERROR) << "isOpened reached unknow state" << sendLog;
	return USEC_SEC;
}


int
Bootes1B::endOpen ()
{
	timeOpenClose = 0;
	return 0;
}


int
Bootes1B::startClose ()
{
	// we cannot close dome when we are still moving
	if (getState () & DOME_CLOSING)
	{
		if (isMoving ())
			return 0;
		return -1;
	}
	if (getState () & DOME_OPENING)
		return -1;

	ZAP(DOMESWITCH);
	sleep (1);
	VYP(DOMESWITCH);
	sleep (1);

	time (&timeOpenClose);
	timeOpenClose += ROOF_TIMEOUT;

	return 0;
}


long
Bootes1B::isClosed ()
{
	time_t now;
	time (&now);
	if (timeOpenClose > 0 && now > timeOpenClose)
	{
		logStream (MESSAGE_ERROR) << "Bootes1B::isClosed dome timeout"
			<< sendLog;
		domeFailed = true;
		// cycle again..
		maskState (DOME_DOME_MASK, DOME_OPENED, "failed closing");
		startClose ();
		return USEC_SEC;
	}
	if (isMoving ())
		return USEC_SEC;
	// at least one switch must be closed
	if (getPortState (CLOSE_END_1) || getPortState (CLOSE_END_2))
		return -2;
	if (getPortState (OPEN_END_1) && getPortState (OPEN_END_2))
		return USEC_SEC;
	logStream (MESSAGE_ERROR) << "isClosed reached unknow state" << sendLog;
	return USEC_SEC;
}


int
Bootes1B::endClose ()
{
	timeOpenClose = 0;
	return 0;
}


int
Bootes1B::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("telon"))
	{
		ZAP (TEL_SWITCH);
		return 0;
	}
	if (conn->isCommand ("teloff"))
	{
		VYP (TEL_SWITCH);
		return 0;
	}
	return Ford::commandAuthorized (conn);
}


int
main (int argc, char **argv)
{
	Bootes1B device = Bootes1B (argc, argv);
	return device.run ();
}
