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
	netinfo_packed_t *packed_cast;
	char *packedbuild;
	short packedbuild_size;
	
	rtinfo_cpu_t *cpu;
	int nbcpu;
	short cpu_size;
	
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
	cpu = rtinfo_init_cpu(&nbcpu);
	free(cpu);
	
	if(nbcpu > 16) {
		nbcpu = 16;
		fprintf(stderr, "[-] Warning: At this time, only 15 cpu can handled over the network\n");
	}
	
	/* Calculate size of packed_t required */
	cpu_size = sizeof(rtinfo_cpu_t) * nbcpu;
	
	/* Building 'netbuild' memory area with dynamic extra content (like interfaces) */
	packedbuild_size = sizeof(netinfo_packed_t) + cpu_size;
	packedbuild = (char*) malloc(packedbuild_size);
	
	/* Casting to avoid shit code */
	packed_cast = (netinfo_packed_t*) packedbuild;
	
	/* Saving nbinterface to sending packet */
	packed_cast->nbcpu = nbcpu;
	
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
	packed_cast->options		= QRY_SOCKET;
	packed_cast->loadavg.load[0]	= CLIENT_VERSION; 
	netinfo_send(sockfd, packed_cast, packedbuild_size, &remote);
	
	/* Waiting response from server */
	if(recv(sockfd, packed_cast, packedbuild_size, 0) == -1)
		diep("recvfrom");
	
	if(!(packed_cast->options & ACK_SOCKET)) {
		fprintf(stderr, "[-] Wrong response from server\n");
		exit(1);
	}
	
	if((int) packed_cast->loadavg.load[0] != (int) CLIENT_VERSION) {
		fprintf(stderr, "[-] Major version client/server (%.2f/%.2f) mismatch\n", CLIENT_VERSION, packed_cast->loadavg.load[0]);
		exit(1);
	}
		
	printf("[+] Client id: %d\n", packed_cast->clientid);
	printf("[+] Server version: %.2f\n", packed_cast->loadavg.load[0]);
	
	
	/*
	 * Building options
	 */
	packed_cast->options         = USE_MEMORY | USE_LOADAVG | USE_TIME;	/* FIXME: Not used at this time */
	netbuild_cast->options = USE_NETWORK;
	
	printf("[+] Sending data...\n");
	
	/* Working */
	while(1) {	
		/* Pre-reading data */
		rtinfo_get_cpu(packed_cast->cpu, packed_cast->nbcpu);
		rtinfo_get_network(truenet, netbuild_cast->nbiface);

		/* Sleeping */
		usleep(UPDATE_INTERVAL);
		
		/* Reading CPU */
		rtinfo_get_cpu(packed_cast->cpu, packed_cast->nbcpu);
		rtinfo_mk_cpu_usage(packed_cast->cpu, packed_cast->nbcpu);
		
		/* Reading Network */
		rtinfo_get_network(truenet, netbuild_cast->nbiface);
		rtinfo_mk_network_usage(truenet, netbuild_cast->nbiface, UPDATE_INTERVAL / 1000);
		
		/* Reading Memory */
		if(!rtinfo_get_memory(&packed_cast->memory))
			return 1;
		
		/* Reading Load Average */
		if(!rtinfo_get_loadavg(&packed_cast->loadavg))
			return 1;
		
		/* Reading uptime */
		if(!rtinfo_get_uptime(&packed_cast->uptime))
			return 1;
		
		/* Reading Battery State */
		if(!rtinfo_get_battery(&packed_cast->battery))
			return 1;
		
		if(!rtinfo_get_temp(&packed_cast->temperature))
			return 1;
		
		/* Reading Time Info */
		time((time_t*) &packed_cast->timestamp);
		
		/* Sending info_t packed */
		netinfo_send(sockfd, packed_cast, packedbuild_size, &remote);
		
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
