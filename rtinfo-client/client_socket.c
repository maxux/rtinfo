#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <endian.h>
#include "../rtinfo-common/socket.h"
#include "client_socket.h"
#include "client.h"

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

int netinfo_send_packed(int sockfd, netinfo_packed_t *packed, size_t size, const struct sockaddr_in *remote) {
	int i;
	
	/* Converting packet for network endian */
	packed->memory.ram_total  = htobe64(packed->memory.ram_total);
	packed->memory.ram_used   = htobe64(packed->memory.ram_used);
	packed->memory.swap_total = htobe64(packed->memory.swap_total);
	packed->memory.swap_free  = htobe64(packed->memory.swap_free);
	
	for(i = 0; i < 3; i++)
		packed->loadavg[i]  = htobe32(packed->loadavg[i]);
	
	packed->battery.charge_full = htobe64(packed->battery.charge_full);
	packed->battery.charge_now  = htobe64(packed->battery.charge_now);
	packed->battery.status      = htobe64(packed->battery.status);
	
	packed->uptime.uptime       = htobe32(packed->uptime.uptime);
	
	packed->temp_cpu.critical    = htobe16(packed->temp_cpu.critical);
	packed->temp_cpu.cpu_average = htobe16(packed->temp_cpu.cpu_average);
	
	packed->temp_hdd.peak        = htobe16(packed->temp_hdd.peak);
	packed->temp_hdd.hdd_average = htobe16(packed->temp_hdd.hdd_average);	
	
	packed->timestamp = htobe32(packed->timestamp);
	
	/* for(i = 0; i < be32toh(packed->nbcpu); i++) {
		
	} */
	
	/* printf("[+] Sending packed (size: %u)\n", size); */
	if(sendto(sockfd, packed, size, 0, (const struct sockaddr *) remote, sizeof(struct sockaddr_in)) == -1)
		perror("sendto");

	return 0;
}

int netinfo_send_packed_net(int sockfd, netinfo_packed_net_t *net, size_t size, const struct sockaddr_in *remote) {
	/* printf("[+] Sending packed (size: %u)\n", size); */
	if(sendto(sockfd, net, size, 0, (const struct sockaddr *) remote, sizeof(struct sockaddr_in)) == -1)
		perror("sendto");

	return 0;
}

void convert_header(netinfo_packed_t *packed) {
	packed->options  = be32toh(packed->options);
	packed->clientid = be32toh(packed->clientid);
	packed->version  = be32toh(packed->version);
}
