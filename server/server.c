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
#include "../socket.h"
#include "server.h"
#include "display.h"
#include "stack.h"

client_t *clients = NULL;

void diep(char *str) {
	perror(str);
	exit(1);
}

void dummy(int signal) {
	switch(signal) {
		// 
	}
}

int main(void) {
	struct sockaddr_in si_me, remote;
	int sockfd, cid = 2;
	size_t slen = sizeof(remote);
	netinfo_packed_t packed;
	client_t *client;
	
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
	
	attrset(COLOR_PAIR(1));
	
	/* Skipping Resize Signal */
	signal(SIGWINCH, dummy);
	
	/* printw("%d %d %d %d %d = %d\n", sizeof(netinfo_options_t), sizeof(info_memory_t), sizeof(info_loadagv_t), sizeof(info_battery_t), sizeof(uint64_t), sizeof(netinfo_packed_t)); */

	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &si_me, 0, sizeof(si_me));
	
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(DEFAULT_PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(sockfd, (struct sockaddr*) &si_me, sizeof(si_me)) == -1)
		diep("bind");
	
	/* Print Header */
	printw(" Hostname   | CPU Usage            | RAM            | SWAP         | Load Avg. | Remote IP \n");
	printw("------------+----------------------+----------------+--------------+-----------+-------------------");
	refresh();

	while(1) {
		if(recvfrom(sockfd, &packed, sizeof(packed), 0, (struct sockaddr *) &remote, &slen) == -1)
			diep("recvfrom()");
		
		if(packed.options & QRY_SOCKET) {
			/* printw("New client: %d\n", clients); */
			
			packed.options  = ACK_SOCKET;
			packed.clientid = cid;
			
			if(sendto(sockfd, &packed, sizeof(netinfo_packed_t), 0, (const struct sockaddr *) &remote, sizeof(struct sockaddr_in)) == -1)
				diep("sendto");
				
			continue;
		}
		
		if(!(client = stack_search(packed.hostname))) {
			client = (client_t*) malloc(sizeof(client_t));
			
			client->id   = cid++;
			strncpy(client->name, packed.hostname, sizeof(client->name));
			client->name[sizeof(client->name) - 1] = '\0';
			
			if(!stack_client(client)) {
				fprintf(stderr, "Stacking client failed\n");
				return 1;
			}
		}
		
		time(&client->last);

		/* Print client */
		show_packet(&packed, &remote, client);
		
		/* Check list */
		stack_ping();
	}

	close(sockfd);
	endwin();
	
	return 0;
}
