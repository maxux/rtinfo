#ifndef __RTINFOD_STACK_H
	#define __RTINFOD_STACK_H
	
	client_t *stack_client(client_t *new);
	client_t *unstack_client(client_t *remove);
	client_t *stack_search(char *name);
	client_t *stack_newclient(char *hostname, uint32_t remoteip);
#endif
