/*
 * rtinfod is the daemon monitoring rtinfo remote clients
 * Copyright (C) 2012  DANIEL Maxime <root@maxux.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <rtinfo.h>
#include <endian.h>
#include "../rtinfo-common/socket.h"
#include "rtinfod.h"
#include "rtinfod_stack.h"
#include "rtinfod_ip.h"
#include "rtinfod_network.h"
#include "rtinfod_input.h"
#include "rtinfod_debug.h"

char *__bind_input[MAX_NETWORK_BIND] = {"0.0.0.0"};
int __bind_input_count = 1;

void *init_input(void *data) {
	thread_input_t *root = (thread_input_t *) data;
	thread_input_t *new[MAX_NETWORK_BIND];
	int i;
	
	for(i = 0; i < __bind_input_count && __bind_input[i]; i++) {
		if(!(new[i] = (thread_input_t *) malloc(sizeof(thread_input_t))))
			diep("[-] input: malloc");
		
		new[i]->port = root->port;
		
		bzero(&new[i]->si_me, sizeof(new[i]->si_me));
		verbose("[+] input: binding to <%s>\n", __bind_input[i]);
		
		new[i]->si_me.sin_family      = AF_INET;
		new[i]->si_me.sin_port        = htons(root->port);
		new[i]->si_me.sin_addr.s_addr = inet_addr(__bind_input[i]);
		
		if((new[i]->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
			diep("[-] input: socket");
		
		if(bind(new[i]->sockfd, (struct sockaddr*) &new[i]->si_me, sizeof(new[i]->si_me)) == -1)
			diep("[-] input: bind");
		
		if(pthread_create(&new[i]->thread, NULL, thread_input, (void *) new[i]))
			diep("[-] input: pthread_create");
	}
	
	for(i = 0; i < __bind_input_count && __bind_input[i]; i++)
		pthread_join(new[i]->thread, NULL);
	
	return data;
}

void *thread_input(void *data) {
	struct sockaddr_in remote;
	int recvsize, sockfd;
	socklen_t slen = sizeof(remote);
	netinfo_packed_t *packed;
	netinfo_packed_net_t *net;
	rtinfo_network_legacy_t *read;
	size_t checklength;
	client_t *client;
	thread_input_t *thread_input = (thread_input_t*) data;
	uint32_t netindex;
	
	void *buffer = malloc(sizeof(char) * BUFFER_SIZE);
	
	// FIXME
	/* ip allowed */
	unsigned int *mask = NULL, *baseip = NULL;
	int nballow = 0, i;
	
	verbose("[+] input: ready, waiting data\n");
	sockfd = thread_input->sockfd; // FIXME
	
	while(1) {
		if((recvsize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &remote, &slen)) == -1)
			perror("recvfrom");
		
		verbose("[+] input: read %u bytes from %s\n", recvsize, inet_ntoa(remote.sin_addr));
		
		if(recvsize < 0)
			continue;
		
		if(global.debug)
			debug_input(buffer, recvsize);
		
		/* Checking ip --allowed */
		if(nballow) {
			for(i = 0; i < nballow; i++) {
				if((remote.sin_addr.s_addr & mask[i]) == baseip[i])
					i = -1;
			}
			
			/* ip disallowed */
			if(i != -1) {
				verbose("Warning: denied packed received from: %s\n", inet_ntoa(remote.sin_addr));				
				continue;
			}
		}
		
		/* Converting header endianess */
		packed = (netinfo_packed_t*) buffer;
		convert_header(packed);
		
		/* Locking globals */
		pthread_mutex_lock(&global.mutex_clients);
		
		/* Reading header */
		if(packed->options & QRY_SOCKET) {
			verbose("[+] input: new client: %s/%s\n", inet_ntoa(remote.sin_addr), packed->hostname);
			
			/* Authentification accepted */
			packed->options  = htobe32(ACK_SOCKET);
			
			/* Sending Server Version */
			packed->version  = htobe32((int)(SERVER_VERSION));
			
			if(sendto(sockfd, buffer, recvsize, 0, (const struct sockaddr *) &remote, sizeof(struct sockaddr_in)) == -1)
				perror("[-] input: sendto");
			
			/* Checking if the client is not already on stack */
			if(!(client = stack_search(packed->hostname)))
				client = stack_newclient(packed->hostname, remote.sin_addr.s_addr);
			
			goto unlock;
		}
		
		/* Version Check */
		if(packed->version != (unsigned int) (SERVER_VERSION)) {
			warning("[-] input: warning: invalid version (%u) from %s (%s)\n", packed->version, inet_ntoa(remote.sin_addr), packed->hostname);
			goto unlock;
		}
		
		/* Checking size consistency */
		if(packed->options == USE_NETWORK) {
			/* Network size, should not exceed 32 bytes per interface name */
			net = (netinfo_packed_net_t*) packed;
			net->nbiface = be32toh(net->nbiface);
			
			checklength = sizeof(netinfo_packed_net_t) + (sizeof(rtinfo_network_legacy_t) * net->nbiface) + (net->nbiface * 32);
			
			if((size_t) recvsize > checklength) {
				warning("[-] input: warning: wrong network summary datasize (%d bytes) from %s, should be <= %zu bytes.\n",
					recvsize, inet_ntoa(remote.sin_addr), checklength);

				goto unlock;
			}
			
		} else {
			/* Summary size */
			checklength = sizeof(netinfo_packed_t) + (sizeof(rtinfo_cpu_legacy_t) * be32toh(packed->nbcpu));
			
			if((size_t) recvsize != checklength) {
				warning("[-] input: warning: wrong summary datasize (%d bytes) from %s, should be %zu bytes.\n",
					recvsize, inet_ntoa(remote.sin_addr), checklength);

				goto unlock;
			}
		}
		
		/* Searching client on stack */
		if(!(client = stack_search(packed->hostname)))
			client = stack_newclient(packed->hostname, remote.sin_addr.s_addr);
		
		/* Saving last update time (timeout) */
		time(&client->lasttime);
		
		if(packed->options == USE_NETWORK) {			
			/* Updating allocation */
			if(client->net_length != (size_t) recvsize) {
				client->net = (netinfo_packed_net_t *) realloc(client->net, recvsize);
				client->net_length = (size_t) recvsize;
			}
			
			memcpy(client->net, buffer, recvsize);
			
			client->net->nbiface = htobe32(client->net->nbiface);
			read = client->net->net;
			
			for(netindex = 0; netindex < client->net->nbiface; netindex++) {
				convert_packed_net(read);
				read = (rtinfo_network_legacy_t*) ((char*) read + sizeof(rtinfo_network_legacy_t) + read->name_length);
			}

		} else {			
			/* Updating allocation */
			if(client->summary_length != (size_t) recvsize) {
				client->summary = (netinfo_packed_t *) realloc(client->summary, recvsize);
				client->summary_length = recvsize;
			}
			
			/* Copy current data */
			memcpy(client->summary, buffer, recvsize);
			
			/* Correct indian */
			convert_packed(client->summary);
		}
		
		/* Unlocking globals */
		unlock:
			pthread_mutex_unlock(&global.mutex_clients);
	}

	close(sockfd);
	
	return data;
}
