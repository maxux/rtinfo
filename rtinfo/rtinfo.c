#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include "../librtinfo/sysinfo.h"
#include "../librtinfo/misc.h"
#include "socket.h"
#include "rtinfo-client.h"
#include "rtinfo-local.h"

int main(int argc, char *argv[]) {
	int port;
	
	/* printf("%d %d %d %d %d\n", sizeof(netinfo_options_t), sizeof(info_memory_t), sizeof(info_loadagv_t), sizeof(info_battery_t), sizeof(uint64_t)); */
	
	if(argc > 1) {
		/* Network Side */
		port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;
		return networkside(argv[1], port);
		
		/* Local Side */
	} else return localside();
}
