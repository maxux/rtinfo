#ifndef __SERVER_H
	#define __SERVER_H
	
	#include <time.h>
	
	#define BUFFER_SIZE		1024
	#define SERVER_VERSION		3.0

	typedef struct client_t {
		int id;
		char name[32];
		time_t last;
		int nbiface;
		int line;
		
		struct client_t *next;
		
	} client_t;
	
	void diep(char *str);
#endif
