#ifndef __SERVER_H
	#define __SERVER_H
	
	#include <time.h>

	typedef struct client_t {
		int id;
		char name[32];
		time_t last;
		
		struct client_t *next;
		
	} client_t;
	
	void diep(char *str);
#endif
