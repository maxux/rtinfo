#ifndef __STACK_H
	#define __STACK_H
	
	client_t * stack_client(client_t *new);
	client_t * unstack_client(client_t *remove);
	client_t * stack_search(char *name);
	void * stack_ping(void *dummy);
#endif
