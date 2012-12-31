#ifndef __RTINFOD_INPUT_H
	#define __RTINFOD_INPUT_H
	
	typedef struct thread_input_t {
		pthread_t thread;
		unsigned int port;
		
	} thread_input_t;
	
	void * thread_input(void *data);
#endif
