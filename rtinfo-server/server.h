#ifndef __SERVER_H
	#define __SERVER_H
	
	#include <time.h>
	
	#define BUFFER_SIZE		65535   /* should not exceed udp limit */
	#define REQUIRED_LIB_VERSION	4.00
	
	#define WINDOW_WIDTH		148

	typedef struct client_t {
		int id;
		char name[32];
		time_t last;
		
		uint32_t dspiface;
		
		struct netinfo_packed_t *summary;
		size_t summary_length;

		struct netinfo_packed_net_t *net;
		size_t net_length;
		
		WINDOW *window;
		WINDOW *netwindow;
		int winx, winy;
		int netx, nety;
		
		struct client_t *next;
		
	} client_t;
	
	void diep(char *str);
	
	extern WINDOW *wdebug;
	extern int nbclients, newnety;
	
	#define debug(format, ...)	{ wmove(wdebug, 0, 0); \
					wprintw(wdebug, format, __VA_ARGS__); \
					wclrtoeol(wdebug); \
					wrefresh(wdebug); }
#endif
