#ifndef __SERVER_H
	#define __SERVER_H
	
	#include <time.h>
	
	#define BUFFER_SIZE		1024
	#define REQUIRED_LIB_VERSION	4.00
	
	#define WINDOW_WIDTH		145

	typedef struct client_t {
		int id;
		char name[32];
		time_t last;
		
		uint32_t nbiface;
		
		struct netinfo_packed_t *summary;
		struct netinfo_packed_net_t *net;
		
		WINDOW *window;
		WINDOW *netwindow;
		int winx, winy;
		int netx, nety;
		
		struct client_t *next;
		
	} client_t;
	
	void diep(char *str);
	
	extern WINDOW *wdebug;
	extern int nbclients, newnety;
	
	#define debug(format, ...)	wmove(wdebug, 0, 0); \
					wprintw(wdebug, format, __VA_ARGS__); \
					wclrtoeol(wdebug); \
					wrefresh(wdebug);
#endif
