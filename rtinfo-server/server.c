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
#include <endian.h>
#include "../rtinfo-common/socket.h"
#include "server.h"
#include "server_display.h"
#include "server_stack.h"
#include "server_ip.h"
#include "server_socket.h"

client_t *clients = NULL, *lastclient = NULL;
int nbclients = 0, newnety = 0;

pthread_mutex_t mutex_clients;

WINDOW *wdebug;

static struct option long_options[] = {
	{"allow",       required_argument, 0, 'a'},
	{"port",        required_argument, 0, 'p'},
	{"help",        no_argument,       0, 'h'},
	{"debug",       no_argument,       0, 'd'},
	{0, 0, 0, 0}
};

void diep(char *str) {
	endwin();
	endwin();
	perror(str);
	exit(1);
}

void dump(unsigned char *data, unsigned int len) {
	unsigned int i;
	
	printf("[+] DATA DUMP\n");
	printf("[ ] 0x0000 == ");
	
	for(i = 0; i < len;) {
		printf("0x%02x ", data[i++]);
		
		if(i % 16 == 0)
			printf("\n[ ] 0x%04x == ", i);
	}
	
	printf("\n");
}

void dummy(int signal) {
	switch(signal) {
		case SIGINT:
			endwin();
			exit(EXIT_SUCCESS);
		break;
		
		case SIGWINCH:
			/* endwin();
			clear();
			refresh_whole();
			refresh();
			refresh_whole(); */	/* Second pass required */
		break;
	}
}

void print_usage(char *app) {
	printf("rtinfo Server (version %u)\n", SERVER_VERSION);
	printf("%s [-p|--port PORT] [-a|--allow IP/MASK] [-h|--help]\n\n", app);
	
	printf(" --port      specify listen port\n");
	printf(" --allow     remote ip allowed (ip/mask)\n");
	printf(" --help      this message\n");
	
	exit(1);
}

/* void * input_handler(void *dummy) {
	int ch = 0;
	int sub;
	client_t *temp;
	time_t t;
	
	usleep(2000000);
	
	while(1) {
		ch = wgetch(clients->window);
			
		switch(ch) {
			case 'd':
				sub = 0;
				
				time(&t);
				
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
} */

client_t * build_headers() {
	client_t *client;
	
	client = (client_t*) malloc(sizeof(client_t));
	lastclient = client;
	clients    = client;
	
	client->id      = 0;
	client->nbiface = 1;
	client->name[0] = '\0';
	client->next    = NULL;
	client->last    = time(NULL);
	
	client->winx      = 0;
	client->winy      = 0;
	client->window    = newwin(2, WINDOW_WIDTH, client->winy, 0);
	
	client->netx      = 0;
	client->nety      = 3;
	client->netwindow = newwin(2, WINDOW_WIDTH, client->nety, 0);
	
	build_header(client->window);
	build_netheader(client->netwindow);
	
	return client;
}

int debug_mode(int sockfd) {
	struct sockaddr_in remote;
	int recvsize;
	socklen_t slen = sizeof(remote);
	void *buffer = malloc(sizeof(char) * BUFFER_SIZE);	/* Data read */
	netinfo_packed_t *cast;
	netinfo_packed_net_t *net;
	uint32_t i;
	
	printf("[+] Debug Mode: waiting...\n");
	
	while(1) {
		if((recvsize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &remote, &slen)) == -1)
			diep("recvfrom");
		
		dump(buffer, recvsize);
		
		cast = (netinfo_packed_t*) buffer;
		printf("[+] Options  : %u\n", be32toh(cast->options));
		printf("[+] Hostname : %s\n", cast->hostname);
		printf("[+] ClientID : %u\n", be32toh(cast->clientid));
		printf("[+] Version  : %u\n\n", be32toh(cast->version));
		
		if(be32toh(cast->options) & QRY_SOCKET) {
			cast->options = htobe32(ACK_SOCKET);
			
			if(sendto(sockfd, buffer, recvsize, 0, (const struct sockaddr *) &remote, sizeof(struct sockaddr_in)) == -1)
				perror("sendto");
			
			continue;
		}
		
		if(be32toh(cast->options) & USE_NETWORK) {
			net = (netinfo_packed_net_t *) cast;
			net->nbiface = be32toh(net->nbiface);
			
			printf("[+] Network packet data:\n");
			printf("[ ] Interfaces: %d\n", net->nbiface);
			
			for(i = 0; i < net->nbiface; i++) {
				printf("[ ] Name: %s [%s]\n", net->net[i].name, net->net[i].ip);
				printf("[ ] Rate: %llu / %llu\n", be64toh(net->net[i].up_rate), be64toh(net->net[i].down_rate));
				printf("[ ] Data: %llu / %llu\n\n", be64toh(net->net[i].current.up), be64toh(net->net[i].current.down));
			}
			
		} else {
			printf("[+] Summary packet data:\n");
			
			printf("[ ] CPU Count  : %u\n", be32toh(cast->nbcpu));
			printf("[ ] Memory RAM : %llu / %llu\n", be64toh(cast->memory.ram_used), be64toh(cast->memory.ram_total));
			printf("[ ] Memory SWAP: %llu / %llu\n", be64toh(cast->memory.swap_free), be64toh(cast->memory.swap_total));
			printf("[ ] Load Avg.  : %.2f / %.2f / %.2f\n", ((float) be32toh(cast->loadavg[0]) / 100), ((float) be32toh(cast->loadavg[1]) / 100), ((float) be32toh(cast->loadavg[2]) / 100));
			printf("[ ] Battery    : %u / %u / %u / %llu\n", be32toh(cast->battery.charge_full), be32toh(cast->battery.charge_now), cast->battery.load, be64toh(cast->battery.status));
			
			printf("[ ] CPU Temp   : %hu / %hu\n", be16toh(cast->temp_cpu.cpu_average), be16toh(cast->temp_cpu.critical));
			printf("[ ] HDD Temp   : %hu / %hu\n", be16toh(cast->temp_hdd.hdd_average), be16toh(cast->temp_hdd.peak));
			
			printf("[ ] Uptime     : %u\n", be32toh(cast->uptime.uptime));
			printf("[ ] Timestamp  : %u\n\n", be32toh(cast->timestamp));
		}
	}
	
	return 0;
}

int main(int argc, char *argv[]) {
	struct sockaddr_in si_me, remote;
	int sockfd, cid = 0, recvsize;
	socklen_t slen = sizeof(remote);
	void *buffer = malloc(sizeof(char) * BUFFER_SIZE);	/* Data read */
	netinfo_packed_t *packed;
	netinfo_packed_net_t *net;
	char debug = 0;
	
	client_t *client;
	pthread_t thread_ping;//, thread_input;
	
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
			
			case 'd':
				debug = 1;
			break;

			default:
				abort();
		}
	}
	
	/* Reset client-id variable */
	cid = 0;
	
	/* Creating socket */
	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(sockfd, (struct sockaddr*) &si_me, sizeof(si_me)) == -1)
		diep("bind");
	
	/* Debug Mode */
	if(debug)
		return debug_mode(sockfd);
	
	/* Init Console */
	initscr();		/* Init ncurses */
	cbreak();		/* No break line */
	noecho();		/* No echo key */
	start_color();		/* Enable color */
	use_default_colors();
	curs_set(0);		/* Disable cursor */
	keypad(stdscr, TRUE);
	scrollok(stdscr, 1);
	
	/* Init Colors */
	init_pair(1, COLOR_WHITE,   COLOR_BLACK);
	init_pair(2, COLOR_BLUE,    COLOR_BLACK);
	init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
	init_pair(4, COLOR_RED,     COLOR_BLACK);
	init_pair(5, COLOR_BLACK,   COLOR_BLACK);
	init_pair(6, COLOR_CYAN,    COLOR_BLACK);
	init_pair(7, COLOR_GREEN,   COLOR_BLACK);
	init_pair(8, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(9, COLOR_WHITE,   COLOR_RED);
	init_color(COLOR_BLACK, 0, 0, 0);
	
	attrset(COLOR_PAIR(1));
	
	/* Handling Resize Signal */
	signal(SIGINT, dummy);
	signal(SIGWINCH, dummy);
	
	/* Starting Ping Thread */
	if(pthread_create(&thread_ping, NULL, stack_ping, NULL))
		diep("pthread_create");
	
	/* Starting CLI Thread */
	/* if(pthread_create(&thread_input, NULL, input_handler, NULL))
		diep("pthread_create"); */
	
	initdisplay();
	build_headers();
	
	while(1) {
		if((recvsize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &remote, &slen)) == -1)
			perror("recvfrom");
		
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
				debug("Warning: denied packed received from: %s", inet_ntoa(remote.sin_addr));				
				continue;
			}
		}
		
		/* Converting endianess */
		packed = (netinfo_packed_t*) buffer;
		convert_header(packed);
		
		/* Reading header */
		if(packed->options & QRY_SOCKET) {
			/* printw("New client: %d | %s\n", cid, packed->hostname); */
			
			/* Authentification accepted */
			packed->options  = htobe32(ACK_SOCKET);
			packed->clientid = htobe32(cid);
			
			/* Sending Server Version */
			packed->version  = htobe32((int)(SERVER_VERSION));
			
			if(sendto(sockfd, buffer, recvsize, 0, (const struct sockaddr *) &remote, sizeof(struct sockaddr_in)) == -1)
				perror("sendto");
			
			/* Checking if the client is not already on stack */
			if(!(client = stack_search(packed->hostname)))
				client = stack_newclient(packed, cid++);
				
			continue;
		}
		
		/* Version Check */
		if(packed->version != (unsigned int) (SERVER_VERSION)) {
			debug("Warning: invalid version (%u) from %s (%s)", packed->version, inet_ntoa(remote.sin_addr), packed->hostname);
			continue;
		}
		
		/* Small Overflow Check */
		if(packed->options != USE_NETWORK && be32toh(packed->nbcpu) > 24) {
			debug("Warning: wrong cpu count (%d) from %s, rejected.", be32toh(packed->nbcpu), inet_ntoa(remote.sin_addr));
			continue;
		}
		
		/* Searching client on stack */
		if(!(client = stack_search(packed->hostname)))
			client = stack_newclient(packed, cid++);
		
		/* Saving last update time (timeout) */
		time(&client->last);
		
		if(packed->options == USE_NETWORK) {
			net = (netinfo_packed_net_t *) buffer;
			convert_packed_net(net);
			show_packet_network(net, client);

		} else {
			convert_packed(packed);
			show_packet(packed, &remote, client);
		}
	}

	close(sockfd);
	endwin();
	
	return 0;
}
