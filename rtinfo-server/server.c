/*
 * rtinfo-server is a ncurses server for monitor multiple rtinfo remote clients
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
#include <ncurses.h>
#include <signal.h>
#include <pthread.h>
#include "../rtinfo/socket.h"
#include "server.h"
#include "display.h"
#include "stack.h"

client_t *clients = NULL;
int nbclients = 0;

void diep(char *str) {
	perror(str);
	exit(1);
}

void dummy(int signal) {
	switch(signal) {
		case SIGINT:
			endwin();
			exit(0);
		break;
		
		case SIGWINCH:
			endwin();
			refresh_whole();
			refresh();
		break;
	}
}

int main(int argc, char *argv[]) {
	struct sockaddr_in si_me, remote;	/* Socket */
	int sockfd, cid = 0, port;		/* Clients */
	size_t slen = sizeof(remote);
	void *buffer = malloc(sizeof(char) * BUFFER_SIZE);	/* Data */
	client_t *client;
	pthread_t ping;
	
	/* Init Port */
	if(argc > 1)
		port = atoi(argv[1]);
		
	else port = DEFAULT_PORT;
	
	/* Init Console */
	initscr();		/* Init ncurses */
	cbreak();		/* No break line */
	noecho();		/* No echo key */
	start_color();		/* Enable color */
	curs_set(0);		/* Disable cursor */
	
	init_pair(1, COLOR_WHITE,  COLOR_BLACK);
	init_pair(2, COLOR_BLUE,   COLOR_BLACK);
	init_pair(3, COLOR_YELLOW, COLOR_BLACK);
	init_pair(4, COLOR_RED,    COLOR_BLACK);
	init_pair(5, COLOR_BLACK,  COLOR_BLACK);
	init_pair(6, COLOR_CYAN,   COLOR_BLACK);
	init_pair(7, COLOR_GREEN,  COLOR_BLACK);
	
	attrset(COLOR_PAIR(1));
	
	/* Skipping Resize Signal */
	signal(SIGINT, dummy);
	signal(SIGWINCH, dummy);
	
	/* printw("%d %d %d %d %d = %d\n", sizeof(netinfo_options_t), sizeof(info_memory_t), sizeof(info_loadagv_t), sizeof(info_battery_t), sizeof(uint64_t), sizeof(netinfo_packed_t)); */

	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(sockfd, (struct sockaddr*) &si_me, sizeof(si_me)) == -1)
		diep("bind");
	
	/* Starting Ping Thread */
	if(pthread_create(&ping, NULL, stack_ping, NULL))
		diep("pthread_create");
	
	show_header();
	
	while(1) {
		if(recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &remote, &slen) == -1)
			diep("recvfrom()");
		
		if(((netinfo_packed_t*) buffer)->options & QRY_SOCKET) {
			/* printw("New client: %d | %s\n", cid, ((netinfo_packed_t*) buffer)->hostname); */
			
			((netinfo_packed_t*) buffer)->options  = ACK_SOCKET;
			((netinfo_packed_t*) buffer)->clientid = cid;
			
			if(sendto(sockfd, buffer, sizeof(netinfo_packed_t), 0, (const struct sockaddr *) &remote, sizeof(struct sockaddr_in)) == -1)
				diep("sendto");
				
			continue;
		}
		
		if(!(client = stack_search(((netinfo_packed_t*) buffer)->hostname))) {
			client = (client_t*) malloc(sizeof(client_t));
			
			client->id      = cid++;
			client->nbiface = 1;
			
			strncpy(client->name, ((netinfo_packed_t*) buffer)->hostname, sizeof(client->name));
			client->name[sizeof(client->name) - 1] = '\0';
			
			if(!stack_client(client)) {
				fprintf(stderr, "Stacking client failed\n");
				return 1;
			}
			
			nbclients++;
			
			show_net_header();
		}
		
		time(&client->last);
		
		if(((netinfo_packed_t*) buffer)->options & USE_NETWORK)	{
			client->nbiface = ((netinfo_packed_net_t*) buffer)->nbiface + 1;
			show_packet_network((netinfo_packed_net_t*) buffer, &remote, client);

		} else show_packet((netinfo_packed_t*) buffer, &remote, client);
		
		/* Check list */
		// stack_ping();
	}

	close(sockfd);
	endwin();
	
	return 0;
}
