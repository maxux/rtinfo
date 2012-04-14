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
#include <getopt.h>
#include <time.h>
#include <rtinfo.h>
#include "socket.h"
#include "server.h"
#include "display.h"
#include "stack.h"
#include "ip.h"

client_t *clients = NULL;
int nbclients = 0;

pthread_mutex_t mutex_clients;

static struct option long_options[] = {
	{"allow",	required_argument, 0, 'a'},
	{"port",	required_argument, 0, 'p'},
	{"help",	no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

void diep(char *str) {
	endwin();
	endwin();
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
			clear();
			refresh_whole();
			refresh();
			refresh_whole();	/* Second pass required */
		break;
	}
}

void print_usage(char *app) {
	printf("rtinfo Server (version %f)\n", SERVER_VERSION);
	printf("%s [-p|--port PORT] [-a|--allow IP/MASK] [-h|--help]\n\n", app);
	
	printf(" --port      specify listen port\n");
	printf(" --allow     remote ip allowed (ip/mask)\n");
	printf(" --help      this message\n");
	
	exit(1);
}

///
/// SIG HANDLER
///
void * input_handler(void *dummy) {
	int ch = 0;
	int sub;
	client_t *temp, *prev;
	time_t t;
	
	while(1) {
		ch = getch();
		
		switch(ch) {
			case 'd':
				sub = 0;
				
				time(&t);
				prev = NULL;
				
				pthread_mutex_lock(&mutex_clients);
				
				temp = clients;
				
				while(temp) {
					temp->id -= sub;
					
					if(t - 30 > temp->last) {
						unstack_client(temp);
						sub++;
					}
				}			

				pthread_mutex_unlock(&mutex_clients);
			break;
		}
	}
	
	return dummy;
}

void error_print(char *message) {
	int x, y;
	
	pthread_mutex_lock(&mutex_clients);
	
	getmaxyx(stdscr, y, x);			
	move(y - 1, 0);
	
	attrset(A_BOLD | COLOR_PAIR(4));
	printw(message);
	attrset(A_BOLD | COLOR_PAIR(1));
	
	clrtoeol();
	refresh();
	
	pthread_mutex_unlock(&mutex_clients);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in si_me, remote;
	int sockfd, cid = 0, recvsize;
	socklen_t slen = sizeof(remote);
	char temp[256];
	void *buffer = malloc(sizeof(char) * BUFFER_SIZE);	/* Data read */
	
	client_t *client;
	pthread_t thread_ping, thread_input;
	
	/* ip allowed */
	unsigned int *mask = NULL, *baseip = NULL;
	int nballow = 0, i;
	int port = DEFAULT_PORT;
	
	int option_index = 0;
	
	if((int) rtinfo_version() != (int) REQUIRED_LIB_VERSION || rtinfo_version() < REQUIRED_LIB_VERSION) {
		fprintf(stderr, "[-] Require librtinfo 3 (>= %.2f)\n", REQUIRED_LIB_VERSION);
		return 1;
	}
	
	/* Parsing options */
	while(1) {
		cid = getopt_long (argc, argv, "a:p:h", long_options, &option_index);

		/* Detect the end of the options. */
		if(cid == -1)
			break;

		switch(cid) {
			/* New allowed client */
			case 'a':
				nballow++;
				
				mask   = (unsigned int*) realloc(mask, sizeof(unsigned int) * nballow);
				baseip = (unsigned int*) realloc(baseip, sizeof(unsigned int) * nballow);
				
				if(ip_parsecidr(optarg, (baseip + (nballow - 1)), (mask + (nballow - 1)))) {
					fprintf(stderr, "[-] cannot read allow entry\n");
					return 1;
				}
			break;
			
			/* Specific port */
			case 'p':
				port = atoi(optarg);
			break;
			
			case 'h':
				print_usage(argv[0]);
			break;

			/* unrecognized option */
			case '?':
				print_usage(argv[0]);
				return 1;
			break;

			default:
				abort();
		}
	}
	
	/* Reset client-id variable */
	cid = 0;
	
	/* Init Console */
	initscr();		/* Init ncurses */
	cbreak();		/* No break line */
	noecho();		/* No echo key */
	start_color();		/* Enable color */
	use_default_colors();
	curs_set(0);		/* Disable cursor */
	keypad(stdscr, TRUE);
	scrollok(stdscr, 1);
	
	init_pair(1, COLOR_WHITE,   COLOR_BLACK);
	init_pair(2, COLOR_BLUE,    COLOR_BLACK);
	init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
	init_pair(4, COLOR_RED,     COLOR_BLACK);
	init_pair(5, COLOR_BLACK,   COLOR_BLACK);
	init_pair(6, COLOR_CYAN,    COLOR_BLACK);
	init_pair(7, COLOR_GREEN,   COLOR_BLACK);
	init_pair(8, COLOR_MAGENTA, COLOR_BLACK);
	init_color(COLOR_BLACK, 0, 0, 0);
	
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
	if(pthread_create(&thread_ping, NULL, stack_ping, NULL))
		diep("pthread_create");
	
	/* Starting CLI Thread */
	/* if(pthread_create(&thread_input, NULL, input_handler, NULL))
		diep("pthread_create"); */
	
	show_header();
	show_net_header();
	
	while(1) {
		if((recvsize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &remote, &slen)) == -1)
			diep("recvfrom()");
		
		/* Checking ip --allowed */
		if(nballow) {
			for(i = 0; i < nballow; i++) {
				if((remote.sin_addr.s_addr & mask[i]) == baseip[i]) {
					i = -1;
					break;
				}
			}
			
			/* ip disallowed */
			if(i != -1) {
				sprintf(temp, "Warning: denied packed received from: %s", inet_ntoa(remote.sin_addr));
				error_print(temp);
				
				continue;
			}
		}
		
		if(((netinfo_packed_t*) buffer)->options & QRY_SOCKET) {
			/* printw("New client: %d | %s\n", cid, ((netinfo_packed_t*) buffer)->hostname); */
			
			/* Authentification accepted */
			((netinfo_packed_t*) buffer)->options  = ACK_SOCKET;
			((netinfo_packed_t*) buffer)->clientid = cid;
			
			/* Sending Server Version */
			((netinfo_packed_t*) buffer)->loadavg.load[0] = SERVER_VERSION;
			
			if(sendto(sockfd, buffer, recvsize, 0, (const struct sockaddr *) &remote, sizeof(struct sockaddr_in)) == -1)
				diep("sendto");
				
			continue;
		}
		
		/* Version Check */
		if((int) ((netinfo_packed_t*) buffer)->version != (int) SERVER_VERSION) {
			sprintf(temp, "Warning: invalid version (%f) from %s", ((netinfo_packed_t*) buffer)->version, inet_ntoa(remote.sin_addr));
			error_print(temp);
			continue;
		}
		
		/* Small Overflow Check */
		if(((netinfo_packed_t*) buffer)->options != USE_NETWORK && ((netinfo_packed_t*) buffer)->nbcpu > 24) {
			sprintf(temp, "Warning: wrong cpu count (%d) from %s, rejected.", ((netinfo_packed_t*) buffer)->nbcpu, inet_ntoa(remote.sin_addr));
			error_print(temp);
			continue;
		}
		
		if(!(client = stack_search(((netinfo_packed_t*) buffer)->hostname))) {
			client = (client_t*) malloc(sizeof(client_t));
			
			client->id      = cid++;
			client->nbiface = 1;
			
			strncpy(client->name, ((netinfo_packed_t*) buffer)->hostname, sizeof(client->name));
			client->name[sizeof(client->name) - 1] = '\0';
			
			if(!stack_client(client)) {
				fprintf(stderr, "[-] stacking client failed\n");
				return 1;
			}
			
			nbclients++;
			
			show_net_header();
		}
		
		time(&client->last);
		
		if(((netinfo_packed_t*) buffer)->options == USE_NETWORK) {
			client->nbiface = ((netinfo_packed_net_t*) buffer)->nbiface + 1;
			show_packet_network((netinfo_packed_net_t*) buffer, &remote, client);

		} else show_packet((netinfo_packed_t*) buffer, &remote, client);
	}

	close(sockfd);
	endwin();
	
	return 0;
}
