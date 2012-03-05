#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "socket.h"
#include "misc.h"

int netinfo_socket(char *server, int port, struct sockaddr_in *remote) {
	int sockfd;

	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) remote, 0, sizeof(struct sockaddr_in));
	
	remote->sin_family = AF_INET;
	remote->sin_port   = htons(port);
	
	if(inet_aton(server, &remote->sin_addr) == 0) {
		fprintf(stderr, "inet_aton: failed\n");
		exit(1);
	}
	
	return sockfd;
}

int netinfo_send(int sockfd, void *packed, size_t size, const struct sockaddr_in *remote) {
	/* printf("Sending packet: %d\n", sizeof(netinfo_packed_t)); */
		
	if(sendto(sockfd, packed, size, 0, (const struct sockaddr *) remote, sizeof(struct sockaddr_in)) == -1)
		diep("sendto()");

	return 0;
}
