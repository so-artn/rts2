/* 
 * ngDome cupola driver.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "dome.h"
#include "connection/tcsng.h"

using namespace rts2dome;

namespace rts2dome
{

/**
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ngDome:public Dome
{
	private:
		//rts2core::ValueInteger * mcount;
		//rts2core::ValueInteger *moveCountTop;
		rts2core::ConnTCSNG * domeconn;
		rts2core::ValueBool *opened;
		rts2core::ValueBool *closed;
		rts2core::ValueString *domeStatus;
		rts2core::ValueBool *toggle;
		HostString *host;
		const char *obsid;
		const char *cfgFile;
		const char *devname;

		
	protected:
		virtual int init()
		{
			host = new HostString ("10.30.3.68", "5750");
			//Hard code obsid and devname bdecase not all options are processed before this is called
			domeconn = new rts2core::ConnTCSNG (this, host->getHostname(), host->getPort(), "BIG61", "UDOME");
			opened->setValueBool(true);

			domeconn->setDebug(false);

			if (domeconn->getDebug())
			{
				logStream(MESSAGE_INFO) << host->getHostname()<< " : " << host->getPort() << " ," << obsid << " ," << devname << sendLog;
			}

			return Dome::init();
		}



		virtual int processOption (int in_opt)
		{
			switch (in_opt)
			{
				case OPT_CONFIG:
					cfgFile = optarg;
					break;
				case 't':
					host = new HostString (optarg, "5750");
					break;
				case 'n':
					devname = optarg;

					break;
				case 'o':
					obsid = optarg;
					break;

				default:
					return Dome::processOption (in_opt);
			}
			return 0;
		}

		virtual int idle()
		{
			std::string resp;
			try
			{
				resp = domeconn->request("STATE");
				resp = strip(resp);
				domeStatus->setValueString( resp );
			}
			catch(rts2core::ConnTimeoutError)
			{
				logStream(MESSAGE_ERROR) << "Timeout error Could not communicate with Upper Dome" << sendLog;
				domeStatus->setValueString("Timeout ERROR");
				resp = "Timeout ERROR";
				throw;
			}
			catch(rts2core::Error)
			{

				logStream(MESSAGE_ERROR) << "Error Could not communicate with Upper Dome" << sendLog;
				domeStatus->setValueString("ERROR");
				resp = "ERROR";
				throw;
			}
			if(resp == "CLOSED")
			{
				closed->setValueBool(true);
				opened->setValueBool(false);
			}
			else if(resp == "OPENED")
			{
				closed->setValueBool(true);
				opened->setValueBool(false);
			}
			else
			{
				closed->setValueBool(false);
				opened->setValueBool(false);
			}
			sendValueAll(domeStatus);

			return Dome::idle();
		}

		virtual int setValue(rts2core::Value *oldValue, rts2core::Value *newValue)
		{
			if(oldValue == toggle)
			{
				
			}
			return Dome::setValue(oldValue, newValue);
		}

		virtual long isMoving ()
		{
			//mcount->inc ();
			//sendValueAll (mcount);
			
			return USEC_SEC;
		}

		virtual int startOpen ()
		{
			rts2core::Connection *telConn = getOpenConnection (DEVICE_TYPE_MOUNT);
			if(!(telConn->getState() & TEL_PARKED))
			{
				logStream(MESSAGE_ERROR) << "Tel must be parked (stowed) to open " << getState() << " " << (TEL_PARKED) <<sendLog;
				
				return -1;
			}
			blockTelMove();
			domeconn->command("OPEN");

			
			return 0;
		}

		virtual long isOpened ()
		{
			std::string resp;
			
			try
			{
				resp = domeconn->request("STATE");
				resp = strip(resp);
				domeStatus->setValueString( resp );
			}
			catch(rts2core::ConnTimeoutError)
			{
				logStream(MESSAGE_ERROR) << "Timeout error Could not communicate with Upper Dome" << sendLog;
				domeStatus->setValueString("Timeout ERROR");
				resp = "Timeout ERROR";
				throw;
			}
			catch(rts2core::Error)
			{

				logStream(MESSAGE_ERROR) << "Error Could not communicate with Upper Dome" << sendLog;
				domeStatus->setValueString("ERROR");
				resp = "ERROR";
				throw;
			}

			sendValueAll(domeStatus);
			if ( domeStatus->getValueString() == "OPENED")
				return -2;
			return USEC_SEC;
		}

		virtual int endOpen ()
		{
			clearTelMove();
			return 0;
		}

		virtual int startClose ()
		{
			
			domeconn->command("CLOSE");
			return 0;
		}

		virtual long isClosed ()
		{
			std::string resp;	
			try
			{
				resp = domeconn->request("STATE");
				resp = strip(resp);
				domeStatus->setValueString( resp );
			}
			catch(rts2core::ConnTimeoutError)
			{
				logStream(MESSAGE_ERROR) << "Timeout error Could not communicate with Upper Dome" << sendLog;
				domeStatus->setValueString("Timeout ERROR");
				resp = "Timeout ERROR";
				throw;
			}
			catch(rts2core::Error)
			{

				logStream(MESSAGE_ERROR) << "Error Could not communicate with Upper Dome" << sendLog;
				domeStatus->setValueString("ERROR");
				resp = "ERROR";
				throw;
			}

			sendValueAll(domeStatus);
			if ( domeStatus->getValueString() == "CLOSED")
			{
				return -2;
			}

			logStream(MESSAGE_INFO) << "not closed?" << sendLog;
			return USEC_SEC;

		}

		virtual int endClose ()
		{
			return 0;
		}
		virtual int info()
		{
			return Dome::info();
		}

	public:
		ngDome (int argc, char **argv):Dome (argc, argv)
		{
			//createValue (mcount, "mcount", "moving count", false);
			//createValue (moveCountTop, "moveCountTop", "move count top", false, RTS2_VALUE_WRITABLE);
			//moveCountTop->setValueInteger (100);
			domeconn = NULL;
			host = NULL;
			obsid = NULL;
			addOption ('t', NULL, 1, "TCS NG hostname[:port]");
			addOption ('n', NULL, 1, "TCS NG telescope device name");
			addOption ('o', NULL, 1, "TCS NG observatory id");
			createValue (domeStatus, "slit_status", "State of the dome slit", true, RTS2_VALUE_WRITABLE);
			createValue (opened, "isOpen", "Is the slit open?", false);
			createValue (closed, "isClosed", "Is the slit closed?", true);
			createValue (toggle, "toggle", "Toggle the dome open or closed", true, RTS2_VALUE_WRITABLE);

			
		}
		virtual int initValues ()
		{
			return Dome::initValues();

		}

		virtual double getSlitWidth (double alt)
		{
			return 1;
		}

		std::string strip(std::string str)
		{
			while( str[0] == ' ')
			{
				str.erase(0, 1);
			}

			while( str[str.length()-1] == '\r' ||  str[str.length()-1] == '\n')
			{
				str=str.erase(str.length()-1, 1);
			}
			return str;
		}
		

};

}

int main (int argc, char **argv)
{
	ngDome device (argc, argv);
	return device.run ();
}
