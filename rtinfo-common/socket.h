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
	
	typedef struct rtinfo_cpu_legacy_t {
		unsigned char usage;                  /* CPU usage (in percent) */
		
	} __attribute__ ((packed)) rtinfo_cpu_legacy_t;
	
	typedef struct netinfo_packed_t {
		/* Common Packed/Net */
		netinfo_options_t options;
		char hostname[32];
		uint32_t clientid;
		uint32_t version;
		
		/* Specific */
		uint32_t nbcpu;
		rtinfo_memory_t memory;
		uint32_t loadavg[3];           /* FIXME: use something else... */
		rtinfo_battery_t battery;
		rtinfo_temp_cpu_t temp_cpu;
		rtinfo_temp_hdd_t temp_hdd;
		rtinfo_uptime_t uptime;
		uint64_t timestamp;
		rtinfo_cpu_legacy_t cpu[];     /* Note: limited to 256 cpu */
		
	} __attribute__ ((packed)) netinfo_packed_t;
	
	typedef struct rtinfo_network_legacy_t {
		char name[32];		/* Interface name */
		struct rtinfo_network_byte_t current;	/* Number of bytes transfered over the interface */
		int64_t up_rate;		/* Upload rate (in b/s) */
		int64_t down_rate;		/* Download rate (in b/s) */
		char ip[16];			/* IP Address in char */
		uint16_t speed;                 /* Link speed in (in Mbps) */
		
	} __attribute__ ((packed)) rtinfo_network_legacy_t;
	
	typedef struct netinfo_packed_net_t {
		/* Common Packed/Net */
		netinfo_options_t options;
		char hostname[32];
		uint32_t nbiface;
		uint32_t version;
		
		/* Specific */
		rtinfo_network_legacy_t net[];	/* Warning: limited to 16 interfaces */
		
	} __attribute__ ((packed)) netinfo_packed_net_t;
#endif
