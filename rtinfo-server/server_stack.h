#ifndef __STACK_H
	#define __STACK_H
	
	client_t * stack_client(client_t *new);
	client_t * unstack_client(client_t *remove);
	client_t * stack_search(char *name);
	client_t * stack_newclient(netinfo_packed_t *buffer, int clientid);
	void * stack_ping(void *dummy);
#endif
