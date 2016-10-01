#ifndef __RTINFO_NETWORK_H
	#define __RTINFO_NETWORK_H

	#define DEFAULT_PORT		9930
	#define DEFAULT_HOST        "localhost"

	int networkside(char *server, int port, int interval, char **disk, unsigned int sdisks);

	#ifdef _WIN32
		#include "windows.h"
		#define usleep(x)		Sleep(x / 1000)
	#endif
#endif
