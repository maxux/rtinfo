#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <rtinfo.h>
#include <time.h>
#include "../rtinfo-common/socket.h"
#include "byte_conversions.h"
#include "client_socket.h"
#include "rtinfo_client.h"

uint8_t rtinfo_speed_packed(uint16_t speed) {
	switch (speed) {
		case 10:
			return NETINFO_NET_SPEED_10;
			
		case 100:
			return NETINFO_NET_SPEED_100;
		
		case 1000:
			return NETINFO_NET_SPEED_1000;
		break;
		
		case 2500:
			return NETINFO_NET_SPEED_2500;
		break;
		
		case 10000:
			return NETINFO_NET_SPEED_10000;
	}
	
	return NETINFO_NET_SPEED_UNK;
}

size_t netbuild_assemble(rtinfo_network_legacy_t *read, rtinfo_network_if_t *intf) {
	struct sockaddr_in ipconv;
	
	/* Just convert data */
	read->current.up   = htobe64(intf->current.up);
	read->current.down = htobe64(intf->current.down);
	read->up_rate      = htobe64(intf->up_rate);
	read->down_rate    = htobe64(intf->down_rate);
	
	/* Legacy convertion from librtinfo speed */
	read->speed        = rtinfo_speed_packed(intf->speed);
	
	/* Reading ip */
	if(inet_pton(AF_INET, intf->ip, &ipconv.sin_addr))
		read->ip = ipconv.sin_addr.s_addr;
		
	else read->ip = 0;
	
	/* Building name */
	read->name_length = strlen(intf->name);
	strncpy(read->name, intf->name, read->name_length);
	
	return sizeof(rtinfo_network_legacy_t) + read->name_length;
}

int netinfo_socket(char *server, int port, struct sockaddr_in *remote) {
	int sockfd;
	struct hostent *hent;
	struct timeval tv = {tv.tv_sec = 2, tv.tv_usec = 0};
	
	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) remote, 0, sizeof(struct sockaddr_in));
	
	remote->sin_family = AF_INET;
	remote->sin_port   = htons(port);
	
	if((hent = gethostbyname(server)) == NULL)
		return -1;
		
	memcpy(&remote->sin_addr, hent->h_addr_list[0], hent->h_length);
	
	/* recv timeout */
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv)))
		diep("[-] setsockopt: SO_RCVTIMEO");
	
	return sockfd;
}

int netinfo_send_packed(int sockfd, netinfo_packed_t *packed, size_t size, const struct sockaddr_in *remote) {
	int i;
	
	/* Converting packet for network endian */
	// packed->nbcpu = htobe32(packed->nbcpu);
	// Note: nbcpu is converted once, on the header build
	
	packed->memory.ram_total  = htobe64(packed->memory.ram_total);
	packed->memory.ram_used   = htobe64(packed->memory.ram_used);
	packed->memory.swap_total = htobe64(packed->memory.swap_total);
	packed->memory.swap_free  = htobe64(packed->memory.swap_free);
	
	for(i = 0; i < 3; i++)
		packed->loadavg[i]  = htobe32(packed->loadavg[i]);
	
	packed->battery.charge_full = htobe32(packed->battery.charge_full);
	packed->battery.charge_now  = htobe32(packed->battery.charge_now);
	packed->battery.status      = htobe64(packed->battery.status);
	
	packed->uptime.uptime       = htobe32(packed->uptime.uptime);
	
	packed->temp_cpu.critical    = htobe16(packed->temp_cpu.critical);
	packed->temp_cpu.cpu_average = htobe16(packed->temp_cpu.cpu_average);
	
	packed->temp_hdd.peak        = htobe16(packed->temp_hdd.peak);
	packed->temp_hdd.hdd_average = htobe16(packed->temp_hdd.hdd_average);	
	
	packed->timestamp = htobe32(packed->timestamp);
	
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
