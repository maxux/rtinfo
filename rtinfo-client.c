#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include "sysinfo.h"
#include "misc.h"
#include "socket.h"
#include "rtinfo-client.h"

int networkside(char *server, int port) {
	netinfo_packed_t packed;
	info_cpu_t *cpu;
	struct sockaddr_in remote;
	/* info_network_t *net;
	int nbiface, i; */

	int sockfd;
	
	/* Writing hostname */
	if(gethostname(packed.hostname, sizeof(packed.hostname)))
		diep("gethostname");
	
	/* Init Socket */
	sockfd = netinfo_socket(server, port, &remote);
	
	/* Authentificating */
	packed.options = QRY_SOCKET;
	netinfo_send(sockfd, &packed, &remote);
	
	/* Waiting response from server */
	if(recv(sockfd, &packed, sizeof(packed), 0) == -1)
		diep("recvfrom");
	
	if(!(packed.options & ACK_SOCKET)) {
		fprintf(stderr, "Wrong response from server\n");
		exit(1);
		
	} else printf("Connected, client id: %d\n", packed.clientid);
	
	/* FIXME: Not used at this time */
	packed.options = USE_MEMORY | USE_LOADAVG | USE_TIME;
	
	/* Initializing variables */
	/* net = initinfo_network(&nbiface); */
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
		/* getinfo_network(net, nbiface); */

		/* Sleeping */
		usleep(UPDATE_INTERVAL);
		
		/* Reading CPU */
		getinfo_cpu(packed.cpu, packed.nbcpu);
		mkinfo_cpu_usage(packed.cpu, packed.nbcpu);
		
		/* Reading Network */
		/* getinfo_network(net, nbiface);
		mkinfo_network_usage(net, nbiface, UPDATE_INTERVAL / 1000); */
		
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
		
		netinfo_send(sockfd, &packed, &remote);
	}
	
	return 0;
}
