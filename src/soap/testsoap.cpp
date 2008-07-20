#include "soaprts2Proxy.h"
#include "rts2.nsmap"

#include "../utils/rts2cliapp.h"

#include <iostream>

class Rts2TestSoap:public Rts2CliApp
{
	private:
		const char *server;
		int next;
		int nowTarget;
		bool setOff;
	protected:
		virtual int processOption (int in_opt);
	public:
		Rts2TestSoap (int in_argc, char **in_argv);
		virtual int doProcessing ();
};

Rts2TestSoap::Rts2TestSoap (int in_argc, char **in_argv):
Rts2CliApp (in_argc, in_argv)
{
	server = "http://localhost:81";
	next = -1;
	nowTarget = -1;
	setOff = false;
	addOption ('s', "server", 1, "soap server (default http://localhost:81");
	addOption ('n', "next", 1, "next target");
	addOption ('N', "now", 1, "now (immediate) target");
	addOption ('o', "set_off", 0, "set centrald to off");
}


int
Rts2TestSoap::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 's':
			server = optarg;
			break;
		case 'n':
			next = atoi (optarg);
			break;
		case 'N':
			nowTarget = atoi (optarg);
			break;
		case 'o':
			setOff = true;
			break;
		default:
			return Rts2CliApp::processOption (in_opt);
	}
	return 0;
}


std::ostream & operator << (std::ostream & _os, rts2__target * in_target)
{
	if (in_target->radec)
	{
		_os << in_target->id << std::endl
			<< in_target->name << std::endl
			<< in_target->type << std::endl
			<< in_target->radec->ra << " " << in_target->radec->dec << std::endl;
	}
	else
	{
		_os << "target not selected" << std::endl;
	}
	return _os;
}


int
Rts2TestSoap::doProcessing ()
{
	struct soap *sp = soap_new ();
	struct rts2__getEquResponse res;
	struct rts2__getTelescopeResponse resTel;
	struct rts2__getExecResponse exec;
	struct rts2__getTargetResponse target;
	struct rts2__getCentraldResponse centrald;
	struct rts2__setCentraldResponse centrald_set;
	struct rts2__getCameraResponse camera_res;
	struct rts2__getCamerasResponse cameras_res;
	struct rts2__setNextResponse nextR;
	struct rts2__setNowResponse nowR;
	int ret;

	soap_init (sp);

	ret = soap_call_rts2__getEqu (sp, server, "", res);
	if (ret != SOAP_OK)
	{
		std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
		return ret;
	}
	std::cout << res.radec->ra << " " << res.radec->dec << std::endl;

	ret = soap_call_rts2__getTelescope (sp, server, "", "T0", resTel);
	if (ret != SOAP_OK)
	{
		std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
		return ret;
	}
	std::cout
		<< resTel.tel->target->ra << " " << resTel.tel->target->dec << std::endl
		<< resTel.tel->mount->ra << " " << resTel.tel->mount->dec << std::endl
		<< resTel.tel->astrometry->ra << " " << resTel.tel->astrometry->
		dec << std::endl << resTel.tel->err->ra << " " << resTel.tel->err->
		dec << std::endl;

	ret = soap_call_rts2__getExec (sp, server, "", exec);
	if (ret != SOAP_OK)
	{
		std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
		return ret;
	}
	std::cout
		<< "Current" << std::endl
		<< exec.current << std::endl
		<< "Next" << std::endl << exec.next << std::endl;
	// << "Priority" << std::endl << exec.priority << std::endl;

	ret = soap_call_rts2__getTarget (sp, server, "", 1, target);
	if (ret != SOAP_OK)
	{
		std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
		return ret;
	}
	std::cout << "Target 1 is " << std::endl << target.target << std::endl;

	ret = soap_call_rts2__getCentrald (sp, server, "", centrald);
	if (ret != SOAP_OK)
	{
		std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
		return ret;
	}
	std::cout << "System " << centrald.system << " daytime: " << centrald.
		daytime << std::endl;

	if (setOff)
	{
		ret =
			soap_call_rts2__setCentrald (sp, server, "", rts2__system__OFF,
			centrald_set);
		if (ret != SOAP_OK)
		{
			std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
			return ret;
		}
	}

	ret = soap_call_rts2__getCamera (sp, server, "", camera_res);
	if (ret != SOAP_OK)
	{
		std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
		return ret;
	}

	ret = soap_call_rts2__getCameras (sp, server, "", cameras_res);
	if (ret != SOAP_OK)
	{
		std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
		return ret;
	}
	std::vector < rts2__camera * >::iterator cam_iter;

	#ifdef WITH_FAST
	for (cam_iter = cameras_res.cameras->camera.begin ();
		cam_iter != cameras_res.cameras->camera.end (); cam_iter++)
	#else
	for (cam_iter = cameras_res.cameras->camera->begin ();
		cam_iter != cameras_res.cameras->camera->end (); cam_iter++)
	#endif
	{
		rts2__camera *cam = (rts2__camera *) * cam_iter;
		std::cout << "Camera " << cam->name << " exposure " << cam->
			exposure << " focuser " << cam->focpos << " filter " << cam->
			filter << std::endl;
	}
	if (next > 0)
	{
		ret = soap_call_rts2__setNext (sp, server, "", next, nextR);
		if (ret != SOAP_OK)
		{
			std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
			return ret;
		}
		std::cout << "Next set to:" << std::endl << nextR.target << std::endl;
	}
	if (nowTarget > 0)
	{
		ret = soap_call_rts2__setNow (sp, server, "", nowTarget, nowR);
		if (ret != SOAP_OK)
		{
			std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
			return ret;
		}
		std::cout << "Now set to:" << std::endl << nowR.target << std::endl;
	}
	soap_free (sp);
	return ret;
}


int
main (int argc, char **argv)
{
	Rts2TestSoap app = Rts2TestSoap (argc, argv);
	return app.run ();
}
