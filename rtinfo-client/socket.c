#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../rtinfo-common/socket.h"
#include "socket.h"
#include "rtinfo-client.h"

int netinfo_socket(char *server, int port, struct sockaddr_in *remote) {
	int sockfd;
	struct hostent *hent;
	
	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) remote, 0, sizeof(struct sockaddr_in));
	
	remote->sin_family = AF_INET;
	remote->sin_port   = htons(port);
	
	if((hent = gethostbyname(server)) == NULL)
		return -1;
		
	memcpy(&remote->sin_addr, hent->h_addr_list[0], hent->h_length);
	
	return sockfd;
}

int netinfo_send(int sockfd, void *packed, size_t size, const struct sockaddr_in *remote) {
	/* printf("Sending packet: %d\n", sizeof(netinfo_packed_t)); */
	
	/* printf("[+] Sending packed (size: %u)\n", size); */
	if(sendto(sockfd, packed, size, 0, (const struct sockaddr *) remote, sizeof(struct sockaddr_in)) == -1)
		diep("sendto()");

	return 0;
}
