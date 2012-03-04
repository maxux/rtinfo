#ifndef __RTSOCKET_H
	#define __RTSOCKET_H
	
	#include "sysinfo.h"
	
	#define DEFAULT_PORT	9930
	
	typedef enum netinfo_options_t {
		QRY_SOCKET	= 1,
		ACK_SOCKET	= 2,
		
		USE_MEMORY	= 4,
		USE_LOADAVG	= 8,
		USE_BATTERY	= 16,
		USE_TIME	= 32
		
	} netinfo_options_t;
	
	typedef struct netinfo_packed_t {
		netinfo_options_t options;
		int clientid		__attribute__ ((packed));
		int nbcpu		__attribute__ ((packed));
		info_cpu_t cpu[16]	__attribute__ ((packed));	/* At this time, limited to 16 CPU */
		info_memory_t memory	__attribute__ ((packed));
		info_loadagv_t loadavg	__attribute__ ((packed));
		info_battery_t battery	__attribute__ ((packed));
		uint64_t timestamp	__attribute__ ((packed));
		char hostname[32];
		
	} netinfo_packed_t;
	
#endif
