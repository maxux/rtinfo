#ifndef __RTINFO_COMMON_H
	#define __RTINFO_COMMON_H
	
	#include <rtinfo.h>
	#include <netinet/in.h>
	
	#define DEFAULT_PORT	9930
	
	typedef enum netinfo_options_t {
		QRY_SOCKET	= 1,
		ACK_SOCKET	= 2,
		
		USE_MEMORY	= 4,
		USE_LOADAVG	= 8,
		USE_BATTERY	= 16,
		USE_TIME	= 32,
		USE_NETWORK	= 64,
		
		USE_DEBUG	= 131072
		
	} netinfo_options_t;
	
	typedef struct netinfo_packed_t {
		/* Common Packed/Net */
		netinfo_options_t options;
		char hostname[32];
		int clientid;
		float version;
		
		/* Specific */
		int nbcpu;
		rtinfo_memory_t memory;
		rtinfo_loadagv_t loadavg;
		rtinfo_battery_t battery;
		rtinfo_temp_cpu_t temp_cpu;
		rtinfo_temp_hdd_t temp_hdd;
		rtinfo_uptime_t uptime;
		uint64_t timestamp;
		rtinfo_cpu_t cpu[];		/* Warning: limited to 15 cpu */
		
	} __attribute__ ((packed)) netinfo_packed_t;
	
	typedef struct netinfo_packed_net_t {
		/* Common Packed/Net */
		netinfo_options_t options;
		char hostname[32];
		int nbiface;
		float version;
		
		/* Specific */
		rtinfo_network_legacy_t net[];	/* Warning: limited to 16 interfaces */
		
	} __attribute__ ((packed)) netinfo_packed_net_t;
#endif
