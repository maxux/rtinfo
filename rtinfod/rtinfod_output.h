#ifndef __RTINFOD_OUTPUT_H
	#define __RTINFOD_OUTPUT_H
	
	typedef struct thread_output_t {
		pthread_t thread;
		unsigned int port;
		int sockfd;
		struct sockaddr_in addr_listen;
		
	} thread_output_t;
	
	extern char *__bind_output[64];
	extern int __bind_output_count;
	
	void *init_output(void *data);
	void *thread_output(void *data);
	void *thread_output_data(void *thread);
#endif
