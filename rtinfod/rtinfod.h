#ifndef __RTINFODD_H
	#define __RTINFODD_H
	
	#define BUFFER_SIZE             65535   /* should not exceed udp limit */
	#define REQUIRED_LIB_VERSION    4.00
	
	#define SERVER_VERSION          0.11
	
	#define MAX_NETWORK_BIND        64

	typedef struct client_t {
		char *name;
		uint32_t remoteip;
		
		time_t lasttime;
		
		struct netinfo_packed_t *summary;
		size_t summary_length;
		
		struct netinfo_packed_net_t *net;
		size_t net_length;
		
		struct client_t *next;
		
	} client_t;
	
	typedef struct global_t {
		client_t *clients;
		pthread_mutex_t mutex_clients;
		
		char debug;
		char verbose;
		
	} global_t;
	
	extern global_t global;
	
	void diep(char *str);
	
	#define debug(...)	{ if(global.debug) { fprintf(stderr, __VA_ARGS__); } }
	#define verbose(...)    { if(global.verbose) { printf( __VA_ARGS__); } }
	#define warning(...)    { if(global.verbose) { printf(__VA_ARGS__); } }
#endif
