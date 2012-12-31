#ifndef __RTINFOD_OUTPUT_H
	#define __RTINFOD_OUTPUT_H
	
	typedef struct thread_output_t {
		pthread_t thread;
		unsigned int port;
		
	} thread_output_t;
	
	void * thread_output(void *data);
	void * thread_output_data(void *thread);
#endif
