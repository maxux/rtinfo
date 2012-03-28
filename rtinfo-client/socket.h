#ifndef __RTSOCKET_H
	#define __RTSOCKET_H
	
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
		USE_NETWORK	= 64
		
	} netinfo_options_t;
	
	typedef struct netinfo_packed_t {
		netinfo_options_t options;
		char hostname[32];
		int clientid			__attribute__ ((packed));
		int nbcpu			__attribute__ ((packed));
		rtinfo_memory_t memory		__attribute__ ((packed));
		rtinfo_loadagv_t loadavg	__attribute__ ((packed));
		rtinfo_battery_t battery	__attribute__ ((packed));
		rtinfo_temp_t temperature	__attribute__ ((packed));
		rtinfo_uptime_t uptime		__attribute__ ((packed));
		uint64_t timestamp		__attribute__ ((packed));
		rtinfo_cpu_t cpu[];		/* Warning: limited to 15 cpu */
		
	} netinfo_packed_t;
	
	typedef struct netinfo_packed_net_t {
		netinfo_options_t options;
		char hostname[32];
		int nbiface;
		rtinfo_network_legacy_t net[];	/* Warning: limited to 16 interfaces */
		
	} netinfo_packed_net_t;
	
	int netinfo_socket(char *server, int port, struct sockaddr_in *remote);
	int netinfo_send(int sockfd, void *packed, size_t size, const struct sockaddr_in *remote);
#endif
