#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ncurses.h>
#include <math.h>
#include "server_ip.h"

int ip_mkmask(int imask) {
	int i, mask = 0;	
	
	for(i = 0; i < imask; i++)
		mask |= (int) pow(2, i);
			
	return mask;
}

int ip_mkip(char *ip) {
	struct sockaddr_in temp;
	
	if(inet_aton(ip, &temp.sin_addr) == 0) {
		fprintf(stderr, "inet_aton: failed\n");
		exit(1);
	}
	
	return temp.sin_addr.s_addr;
}

int ip_parsecidr(char *input, unsigned int *ip, unsigned int *mask) {
	char temp[16];
	unsigned int i;
	
	i = 0;
	while(*(input + i) && *(input + i) != '/')
		i++;
	
	/* Not an IPv4 */
	if(i > sizeof(temp))
		return 1;
	
	strncpy(temp, input, i);
	temp[i] = '\0';
	
	*ip   = ip_mkip(temp);	
	*mask = ip_mkmask(atoi(input + i + 1));
	
	/* Assume that null mask means single ip */
	if(*mask == 0)
		*mask = 32;
	
	return 0;
}
