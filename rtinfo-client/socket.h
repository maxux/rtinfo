#ifndef __RTSOCKET_H
	#define __RTSOCKET_H
	
	int netinfo_socket(char *server, int port, struct sockaddr_in *remote);
	int netinfo_send(int sockfd, void *packed, size_t size, const struct sockaddr_in *remote);
#endif
