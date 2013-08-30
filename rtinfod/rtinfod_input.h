#ifndef __RTINFOD_INPUT_H
	#define __RTINFOD_INPUT_H
	
	#include <netinet/in.h>
	
	typedef struct thread_input_t {
		pthread_t thread;
		unsigned int port;
		int sockfd;
		struct sockaddr_in si_me;
		
	} thread_input_t;
	
	extern char *__bind_input[64];
	extern int __bind_input_count;
	
	void *init_input(void *data);
	void *thread_input(void *data);
#endif
