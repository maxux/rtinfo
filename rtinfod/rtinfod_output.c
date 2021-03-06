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
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <rtinfo.h>
#include <signal.h>
#include <netdb.h>
#include <sys/wait.h>
#include "../rtinfo-common/socket.h"
#include "rtinfod.h"
#include "rtinfod_stack.h"
#include "rtinfod_ip.h"
#include "rtinfod_network.h"
#include "rtinfod_input.h"
#include "rtinfod_debug.h"
#include "rtinfod_output.h"
#include "rtinfod_output_json.h"

char *__bind_output[64] = {"0.0.0.0"};
int __bind_output_count = 1;

typedef struct thread_data_t {
	int sockfd;
	struct sockaddr_in addr_client;
	pthread_t thread;
	
} thread_data_t;

int yes = 1;
char *httpheader = "HTTP/1.0 200 OK\r\n" \
                   "Content-type: application/json\r\n" \
                   "Content-length: %d\r\n\r\n";

void *init_output(void *data) {
	thread_output_t *root = (thread_output_t *) data;;
	thread_output_t *new[MAX_NETWORK_BIND];
	int i;
	
	for(i = 0; i < __bind_output_count && __bind_output[i]; i++) {
		if(!(new[i] = (thread_output_t *) malloc(sizeof(thread_output_t))))
			diep("[-] input: malloc");
		
		new[i]->port = root->port;

		verbose("[+] output: binding to [%s], port: %d\n", __bind_output[i], root->port);

		new[i]->addr_listen.sin_family = AF_INET;
		new[i]->addr_listen.sin_port   = htons(root->port);		
		
		if((new[i]->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			diep("[-] output: socket");

		if(setsockopt(new[i]->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
			diep("[-] output: setsockopt");
		
		new[i]->addr_listen.sin_addr.s_addr = inet_addr(__bind_output[i]);
		
		if(bind(new[i]->sockfd, (struct sockaddr*) &new[i]->addr_listen, sizeof(new[i]->addr_listen)) == -1)
			diep("[-] output: bind");
		
		if(listen(new[i]->sockfd, 32) == -1)
			diep("[-] output: listen");
		
		if(pthread_create(&new[i]->thread, NULL, thread_output, (void *) new[i]))
			diep("pthread_create");
	}
	
	for(i = 0; i < __bind_output_count && __bind_output[i]; i++)
		pthread_join(new[i]->thread, NULL);
	
	return data;
}

void *thread_output(void *data) {
	int new_fd;
	struct sockaddr_in addr_client;
	socklen_t addr_client_len;
	char *client_ip;
	thread_data_t *thread_data;
	thread_output_t *thread_output = (thread_output_t*) data;

	verbose("[+] output: ready, waiting data\n");
	addr_client_len = sizeof(addr_client);
	
	while(1) {
		if((new_fd = accept(thread_output->sockfd, (struct sockaddr *)&addr_client, &addr_client_len)) == -1)
			perror("[-] output: accept");

		client_ip = inet_ntoa(addr_client.sin_addr);
		verbose("[+] output: connection from %s\n", client_ip);
		
		/* Warning: memory leak */
		thread_data = (thread_data_t*) malloc(sizeof(thread_data_t));
		
		thread_data->sockfd      = new_fd;
		thread_data->addr_client = addr_client;
		
		/* Starting new thread with our new client */
		if(pthread_create(&thread_data->thread, NULL, thread_output_data, (void *) thread_data))
			diep("[-] output: pthread_create");
		
		if(pthread_detach(thread_data->thread))
			diep("[-] output: pthread_detach");
	}

	return data;
}

void *thread_output_data(void *thread) {
	thread_data_t *thread_data = (thread_data_t*) thread;
	char buffer[2048], *json;
	int length;
	
	verbose("[+] output: thread %d: started\n", thread_data->sockfd);
	
	while((length = recv(thread_data->sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
		if(length < 6)
			continue;
			
		buffer[length] = '\0';
		
		if(strncmp(buffer, "GET /", 5))
			continue;
		
		if(!strncmp(buffer + 5, "json", 4)) {
			json = output_json();
			
			debug("[ ] output: thread %d: json output (%d bytes):\n", thread_data->sockfd, length);
			debug("%s\n", json);
			
			length = strlen(json);
			
			/* HTTP Header */
			sprintf(buffer, httpheader, length);
			send(thread_data->sockfd, buffer, strlen(buffer), 0);
			
			/* Data */
			send(thread_data->sockfd, json, length, 0);
			free(json);
			
			break;
		}
	}
		
	verbose("[+] output: thread %d: closing\n", thread_data->sockfd);
	
	close(thread_data->sockfd);
	
	/* Clearing */
	free(thread);
	
	return NULL;
}
