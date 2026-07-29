#ifndef __RTINFO_NETWORK_H
	#define __RTINFO_NETWORK_H

	#define DEFAULT_PORT		9930
	#define DEFAULT_HOST        "localhost"

	int networkside(char *server, int port, int interval, char **xdisks, size_t sdisks);
#endif
