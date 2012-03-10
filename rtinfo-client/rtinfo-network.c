#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <rtinfo.h>
#include "socket.h"
#include "rtinfo-network.h"
#include "rtinfo-client.h"

/* void dump(char *s, int j) {
	int i;
	
	for(i = 0; i < j; i++) {
		printf("% 4d ", s[i]);
		if(i % 32 == 0)
			printf("\n");
	}
	
	printf("\n");
} */

int networkside(char *server, int port) {
	netinfo_packed_t packed;
	rtinfo_cpu_t *cpu;
	
	rtinfo_network_t *truenet;
	int nbiface;
	short legacy_size;
	
	char *netbuild;
	short netbuild_size;
	netinfo_packed_net_t *netbuild_cast;
	
	struct sockaddr_in remote;

	int sockfd, i;
	
	printf("[+] Starting rtinfo network client (version %.2f)\n", CLIENT_VERSION);
	printf("[ ] Compiled with librtinfo version %.2f\n", rtinfo_version());
	
	/* Writing hostname on netinfo_packed_t */
	if(gethostname(packed.hostname, sizeof(packed.hostname)))
		diep("gethostname");
	
	/*
	 * Initializing Network
	 */
	truenet = rtinfo_init_network(&nbiface);
	
	if(nbiface > 16) {
		nbiface = 16;
		fprintf(stderr, "[-] Warning: At this time, only 16 network interfaces can handled over the network\n");
	}
	
	/* Calculate size of legacy_t required */
	legacy_size = sizeof(rtinfo_network_legacy_t) * nbiface;
	
	/* Building 'netbuild' memory area with dynamic extra content
	
		<---    netinfo_packed_net_t      ---> <--- [info_network_legacy_t 1][info_network_legacy_t 2][...] --->
		<--- sizeof(netinfo_packed_net_t) ---> <---         sizeof(info_network_legacy_t) * nbifaces        --->
	*/
	netbuild_size = sizeof(netinfo_packed_net_t) + legacy_size;
	netbuild = (char*) malloc(netbuild_size);
	
	/* Casting to avoid shit code */
	netbuild_cast = (netinfo_packed_net_t*) netbuild;
	
	/* Saving nbinterface to sending packet */
	netbuild_cast->nbiface = nbiface;
	
	printf("[+] Network Interfaces  : %d\n", netbuild_cast->nbiface);
	printf("[+] Netinfo summary size: %lu bytes\n", (unsigned long int) sizeof(netinfo_packed_t));
	printf("[+] Netinfo network size: %d bytes\n", netbuild_size);
	
	/* Writing hostname to sending packed */
	if(gethostname(netbuild_cast->hostname, sizeof(netbuild_cast->hostname)))
		diep("gethostname");
	
	
	
	/*
	 * Initializing CPU
	 */
	cpu = rtinfo_init_cpu(&packed.nbcpu);
	free(cpu);
	
	if(packed.nbcpu > 16) {
		fprintf(stderr, "At this time, I don't support > 16 CPU over the network\n");
		exit(1);
	}
	
	/*
	 * Initializing Socket
	 */
	printf("[+] Connecting [%s:%d]...\n", server, port);
	
	if((sockfd = netinfo_socket(server, port, &remote)) == -1) {
		fprintf(stderr, "[-] Cannot resolve host: %s\n", server);
		exit(1);
	}
	
	/* Authentificating */
	packed.options		= QRY_SOCKET;
	packed.loadavg.load[0]	= CLIENT_VERSION; 
	netinfo_send(sockfd, &packed, sizeof(netinfo_packed_t), &remote);
	
	/* Waiting response from server */
	if(recv(sockfd, &packed, sizeof(packed), 0) == -1)
		diep("recvfrom");
	
	if(!(packed.options & ACK_SOCKET)) {
		fprintf(stderr, "[-] Wrong response from server\n");
		exit(1);
	}
	
	if((int) packed.loadavg.load[0] != (int) CLIENT_VERSION) {
		fprintf(stderr, "[-] Major version client/server mismatch\n");
		exit(1);
	}
		
	printf("[+] Client id: %d\n[+] Server version: %.2f\n", packed.clientid, packed.loadavg.load[0]);
	
	
	/*
	 * Building options
	 */
	packed.options         = USE_MEMORY | USE_LOADAVG | USE_TIME;	/* FIXME: Not used at this time */
	netbuild_cast->options = USE_NETWORK;
	
	printf("[+] Sending data...\n");
	
	/* Working */
	while(1) {	
		/* Pre-reading data */
		rtinfo_get_cpu(packed.cpu, packed.nbcpu);
		rtinfo_get_network(truenet, netbuild_cast->nbiface);

		/* Sleeping */
		usleep(UPDATE_INTERVAL);
		
		/* Reading CPU */
		rtinfo_get_cpu(packed.cpu, packed.nbcpu);
		rtinfo_mk_cpu_usage(packed.cpu, packed.nbcpu);
		
		/* Reading Network */
		rtinfo_get_network(truenet, netbuild_cast->nbiface);
		rtinfo_mk_network_usage(truenet, netbuild_cast->nbiface, UPDATE_INTERVAL / 1000);
		
		/* Reading Memory */
		if(!rtinfo_get_memory(&packed.memory))
			return 1;
		
		/* Reading Load Average */
		if(!rtinfo_get_loadavg(&packed.loadavg))
			return 1;
		
		/* Reading Battery State */
		/* if(!rtinfo_get_battery(&battery))
			return 1; */
		
		/* Reading Time Info */
		time((time_t*) &packed.timestamp);
		
		/* Sending info_t packed */
		netinfo_send(sockfd, &packed, sizeof(netinfo_packed_t), &remote);
		
		/*
		 * Building Network packet from librtinfo reponse
		 */
		for(i = 0; i < netbuild_cast->nbiface; i++) {
			strcpy(netbuild_cast->net[i].name, truenet[i].name);
			
			netbuild_cast->net[i].current   = truenet[i].current;
			netbuild_cast->net[i].up_rate   = truenet[i].up_rate;
			netbuild_cast->net[i].down_rate = truenet[i].down_rate;

			strcpy(netbuild_cast->net[i].ip, truenet[i].ip);
		}
		
		/* dump(netbuild, netbuild_size); */
		
		/* Sending info_net_t Packet */
		netinfo_send(sockfd, netbuild, netbuild_size, &remote);
	}
	
	return 0;
}
