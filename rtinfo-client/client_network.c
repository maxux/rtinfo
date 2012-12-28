#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <rtinfo.h>
#include <ctype.h>
#include "../rtinfo-common/socket.h"
#include "byte_conversions.h"
#include "client_socket.h"
#include "client_network.h"
#include "client.h"

void dump(unsigned char *data, unsigned int len) {
	unsigned int i, j;
	
	printf("[+] Data dump [%p -> %p] (%u bytes)\n", data, data + len, len);
	printf("[ ] 0x0000: ");
	
	for(i = 0; i < len;) {
		printf("0x%02x ", data[i++]);
		
		if(i % 16 == 0) {
			printf("|");

			for(j = i - 16; j < i; j++)
				printf("%c", ((isalnum(data[j]) ? data[j] : '.')));

			printf("|\n[ ] 0x%04x: ", i);
		}
	}
	
	if(i % 16) {
		printf("%-*s", 5 * (16 - (i % 16)), " ");

		printf("|");

		for(j = i - (i % 16); j < len; j++)
			printf("%c", ((isalnum(data[j]) ? data[j] : '.')));

		printf("%-*s|\n", 16 - (len % 16), " ");
	}
}

int networkside(char *server, int port) {
	netinfo_packed_t *packed_cast;
	char *packedbuild;
	short packedbuild_size;
	rtinfo_loadagv_t legacy_loadavg;
	
	rtinfo_cpu_t *cpu;
	short cpu_size;
	
	rtinfo_network_t *truenet;
	short legacy_size;
	
	char *netbuild = NULL;
	short netbuild_size = 0;
	short netbuild_effective_size = 0;
	netinfo_packed_net_t *netbuild_cast = NULL;
	rtinfo_network_legacy_t *readnet;
	size_t netsize;
	
	struct sockaddr_in remote;
	int sockfd;
	unsigned int i, oldnbiface;
	
	printf("[+] Starting rtinfo network client (version %u)\n", CLIENT_VERSION);
	printf("[ ] Using librtinfo version %.2f\n", rtinfo_version());
	
	printf("[ ] netinfo_packed_t       : %u bytes\n", sizeof(netinfo_packed_t));
	printf("[ ] netinfo_packed_net_t   : %u bytes\n", sizeof(netinfo_packed_net_t));
	printf("[ ] rtinfo_network_legacy_t: %u bytes\n", sizeof(rtinfo_network_legacy_t));

	/*
	 * Initializing Network
	 */
	truenet = rtinfo_init_network();
	oldnbiface = -1;
	

	/*
	 * Initializing CPU
	 */
	cpu = rtinfo_init_cpu();
	
	/* Calculate size of packed_t required */
	cpu_size = sizeof(rtinfo_cpu_legacy_t) * cpu->nbcpu;
	
	/* Building 'netbuild' memory area with dynamic extra content (like interfaces) */
	packedbuild_size = sizeof(netinfo_packed_t) + cpu_size;
	packedbuild = (char*) malloc(packedbuild_size);
	
	printf("[ ] packedbuild_size: %u bytes\n", packedbuild_size);

	/* Casting to avoid shit code */
	packed_cast = (netinfo_packed_t*) packedbuild;
	
	/* Saving nbinterface to sending packet */
	packed_cast->nbcpu = htobe32(cpu->nbcpu);
	
	/* Writing hostname on netinfo_packed_t */
	if(gethostname(packed_cast->hostname, sizeof(packed_cast->hostname)))
		diep("gethostname");
	
	/*
	 * Initializing Socket
	 */
	printf("[+] Connecting [%s:%d]...\n", server, port);
	
	if((sockfd = netinfo_socket(server, port, &remote)) == -1) {
		fprintf(stderr, "[-] Cannot resolve host: %s\n", server);
		exit(1);
	}
	
	/* Authentificating */
	packed_cast->options = htobe32(QRY_SOCKET);
	packed_cast->version = htobe32(CLIENT_VERSION);
	
	netinfo_send_packed(sockfd, packed_cast, packedbuild_size, &remote);
	
	/* Waiting response from server */
	if(recv(sockfd, packed_cast, packedbuild_size, 0) == -1)
		diep("recvfrom");
	
	convert_header(packed_cast);
	
	/* Checking ACK answer */
	if(!(packed_cast->options & ACK_SOCKET)) {
		fprintf(stderr, "[-] Wrong response from server\n");
		exit(EXIT_FAILURE);
	}
	
	/* Checking version */
	if(packed_cast->version != CLIENT_VERSION) {
		fprintf(stderr, "[-] Version client/server (%u/%u) mismatch\n", CLIENT_VERSION, packed_cast->version);
		exit(EXIT_FAILURE);
	}
		
	printf("[+] Client id: %d\n", packed_cast->clientid);
	printf("[+] Server version: %u\n", packed_cast->version);
	
	
	/*
	 * Building options
	 */
	packed_cast->options   = htobe32(0);	/* FIXME: Not used at this time */
	packed_cast->version   = htobe32(CLIENT_VERSION);
	
	printf("[+] Sending data...\n");
	
	/* Pre-reading data */
	rtinfo_get_cpu(cpu);
	rtinfo_get_network(truenet);
	
	/* Working */
	while(1) {
		/* Sleeping */
		usleep(UPDATE_INTERVAL);

		/* Reading CPU */
		rtinfo_get_cpu(cpu);
		rtinfo_mk_cpu_usage(cpu);
		
		for(i = 0; i < cpu->nbcpu; i++)
			packed_cast->cpu[i].usage = cpu->dev[i].usage;
		
		/* Reading Network */
		rtinfo_get_network(truenet);
		rtinfo_mk_network_usage(truenet, UPDATE_INTERVAL / 1000);
		
		if(!rtinfo_get_memory(&packed_cast->memory))
			return 1;
		
		if(!rtinfo_get_loadavg(&legacy_loadavg))
			return 1;
		
		for(i = 0; i < 3; i++)
			packed_cast->loadavg[i] = (uint32_t)(legacy_loadavg.load[i] * 100);
		
		if(!rtinfo_get_uptime(&packed_cast->uptime))
			return 1;
		
		if(!rtinfo_get_battery(&packed_cast->battery, NULL))
			return 1;
		
		if(!rtinfo_get_temp_cpu(&packed_cast->temp_cpu))
			return 1;
		
		if(!rtinfo_get_temp_hdd(&packed_cast->temp_hdd))
			return 1;
		
		/* Reading Time Info */
		time((time_t*) &packed_cast->timestamp);
		
		/* Sending info_t packed */
		netinfo_send_packed(sockfd, packed_cast, packedbuild_size, &remote);

		/*
		 * Building Network packet from librtinfo response
		 */
		
		/* Calculate size of legacy_t required */
		if(oldnbiface != truenet->nbiface) {
			free(netbuild);
			
			/* Allocating bytes for struct and interface name */
			legacy_size = (sizeof(rtinfo_network_legacy_t) + IFNAMSIZ + 1) * truenet->nbiface;
			
			/* Building 'netbuild' memory area with dynamic extra content
			
				<---    netinfo_packed_net_t      ---> <--- [info_network_legacy_t 1][info_network_legacy_t 2][...] --->
				<--- sizeof(netinfo_packed_net_t) ---> <---         sizeof(info_network_legacy_t) * nbifaces        --->
			*/
			netbuild_size = sizeof(netinfo_packed_net_t) + legacy_size;
			netbuild = (char*) malloc(netbuild_size);
			
			printf("[ ] netbuild_size: %u bytes\n", netbuild_size);

			/* Casting for clean code */
			netbuild_cast = (netinfo_packed_net_t*) netbuild;
			
			/* Writing hostname to sending packed */
			if(gethostname(netbuild_cast->hostname, sizeof(netbuild_cast->hostname)))
				diep("gethostname");
			
			netbuild_cast->options = htobe32(USE_NETWORK);
			netbuild_cast->version = htobe32(CLIENT_VERSION);
			netbuild_cast->nbiface = htobe32(truenet->nbiface);
			
			/* Saving value */
			oldnbiface = truenet->nbiface;
		}

		/* Formating data */
		netbuild_effective_size = sizeof(netinfo_packed_net_t) + (sizeof(rtinfo_network_legacy_t) * truenet->nbiface);
		readnet = netbuild_cast->net;

		for(i = 0; i < truenet->nbiface; i++) {
			netsize = netbuild_assemble(readnet, (truenet->net + i));
			netbuild_effective_size += readnet->name_length;
			
			/* dump(readnet, netsize); */

			readnet = (rtinfo_network_legacy_t*) ((char*) readnet + netsize);
		}
		
		/* dump(netbuild_cast, netbuild_effective_size); */
		
		/* Sending info_net_t Packet */
		netinfo_send_packed_net(sockfd, netbuild_cast, netbuild_effective_size, &remote);
	}
	
	return 0;
}
