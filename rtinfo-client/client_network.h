#ifndef __RTINFO_NETWORK_H
	#define __RTINFO_NETWORK_H
	
	#define UPDATE_INTERVAL		1000000
	#define DEFAULT_PORT		9930

	int networkside(char *server, int port);
	
	#ifdef _WIN32
		#include "windows.h"
		#define usleep(x)		Sleep(x / 1000)
	#endif
#endif
