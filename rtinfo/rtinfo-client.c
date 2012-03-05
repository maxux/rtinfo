#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include "../librtinfo/sysinfo.h"
#include "../librtinfo/misc.h"
#include "socket.h"
#include "rtinfo-client.h"

int networkside(char *server, int port) {
	netinfo_packed_t packed;
	netinfo_packed_net_t net;
	info_cpu_t *cpu;
	info_network_t *truenet;
	struct sockaddr_in remote;

	int sockfd, i;
	
	/* Writing hostname */
	if(gethostname(packed.hostname, sizeof(packed.hostname)))
		diep("gethostname");
	
	if(gethostname(net.hostname, sizeof(net.hostname)))
		diep("gethostname");
	
	/* Init Socket */
	printf("Connecting... ");
	fflush(stdout);
	
	sockfd = netinfo_socket(server, port, &remote);
	
	/* Authentificating */
	packed.options = QRY_SOCKET;
	netinfo_send(sockfd, &packed, sizeof(netinfo_packed_t), &remote);
	
	/* Waiting response from server */
	if(recv(sockfd, &packed, sizeof(packed), 0) == -1)
		diep("recvfrom");
	
	if(!(packed.options & ACK_SOCKET)) {
		fprintf(stderr, "Wrong response from server\n");
		exit(1);
		
	} else printf("ok. Client id: %d\n", packed.clientid);
	
	packed.options = USE_MEMORY | USE_LOADAVG | USE_TIME;	/* FIXME: Not used at this time */
	net.options    = USE_NETWORK;
	
	/* Initializing variables */
	truenet = initinfo_network(&net.nbiface);
	
	if(net.nbiface > 16) {
		fprintf(stderr, "At this time, I don't support > 16 Network interfaces over the network\n");
		exit(1);
	}
	
	cpu = initinfo_cpu(&packed.nbcpu);
	free(cpu);
	
	if(packed.nbcpu > 16) {
		fprintf(stderr, "At this time, I don't support > 16 CPU over the network\n");
		exit(1);
	}
	
	/* Working */	
	while(1) {	
		/* Pre-reading data */
		getinfo_cpu(packed.cpu, packed.nbcpu);
		getinfo_network(truenet, net.nbiface);

		/* Sleeping */
		usleep(UPDATE_INTERVAL);
		
		/* Reading CPU */
		getinfo_cpu(packed.cpu, packed.nbcpu);
		mkinfo_cpu_usage(packed.cpu, packed.nbcpu);
		
		/* Reading Network */
		getinfo_network(truenet, net.nbiface);
		mkinfo_network_usage(truenet, net.nbiface, UPDATE_INTERVAL / 1000);
		
		/* Reading Memory */
		if(!getinfo_memory(&packed.memory))
			return 1;
		
		/* Reading Load Average */
		if(!getinfo_loadavg(&packed.loadavg))
			return 1;
		
		/* Reading Battery State */
		/* if(!getinfo_battery(&battery))
			return 1; */
		
		/* Reading Time Info */
		time((time_t*) &packed.timestamp);
		
		netinfo_send(sockfd, &packed, sizeof(netinfo_packed_t), &remote);
		
		/* Building Net Packet */
		for(i = 0; i < net.nbiface; i++) {
			strcpy(net.net[i].name, truenet[i].name);
			
			net.net[i].current  = truenet[i].current;
			
			net.net[i].up_rate   = truenet[i].up_rate;
			net.net[i].down_rate = truenet[i].down_rate;
			
			strcpy(net.net[i].ip, truenet[i].ip);
		}
		
		netinfo_send(sockfd, &net, sizeof(netinfo_packed_net_t), &remote);
	}
	
	return 0;
}
