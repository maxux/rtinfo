#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <ncurses.h>
#include "../socket.h"
#include "server.h"
#include "display.h"

extern int nbclients;

void show_packet(netinfo_packed_t *packed, struct sockaddr_in *remote, client_t *client) {
	int i;
	float memory_percent, swap_percent;
	
	move(client->id + 2, 0);
	
	attrset(COLOR_PAIR(1));
	
	packed->hostname[14] = '\0';
	printw(" %-14s", packed->hostname);
	
	/* Print CPU Usage */
	printw(" | ");
	
	for(i = 1; i < packed->nbcpu; i++) {
		if(packed->cpu[i].usage > 85)
			attrset(A_BOLD | COLOR_PAIR(4));
			
		else if(packed->cpu[i].usage > 50)
			attrset(A_BOLD | COLOR_PAIR(3));
			
		printw("%3d%% ", packed->cpu[i].usage);
		attrset(COLOR_PAIR(1));	
	}
	
	if(packed->nbcpu < 4)
		for(; i < 5; i++)
			printw("     ");
	
	/* Print Memory Usage */
	printw(" | ");
	memory_percent = ((float) packed->memory.ram_used / packed->memory.ram_total) * 100;
	
	if(memory_percent > 80)
		attrset(A_BOLD | COLOR_PAIR(4));
		
	else if(memory_percent > 50)
		attrset(A_BOLD | COLOR_PAIR(3));
	
	printw("%5lld Mo (%2.0f%%)", packed->memory.ram_used / 1024, memory_percent);
	attrset(COLOR_PAIR(1));
	
	printw(" | ");
	if(packed->memory.swap_total > 0) {
		swap_percent = ((float) (packed->memory.swap_total - packed->memory.swap_free) / packed->memory.swap_total) * 100;
		
		if(swap_percent > 80)
			attrset(A_BOLD | COLOR_PAIR(4));
			
		else if(swap_percent > 50)
			attrset(A_BOLD | COLOR_PAIR(3));
			
		printw("%3lld Mo (%2.0f%%)", (packed->memory.swap_total - packed->memory.swap_free) / 1024, swap_percent);
		
	} else {
		attrset(A_BOLD | COLOR_PAIR(5));
		printw("   No swap  ");
	}
	attrset(COLOR_PAIR(1));
	
	/* Print Load Average Usage */
	printw(" | ");
	
	if(packed->loadavg.min_1 > 1)
		attrset(A_BOLD | COLOR_PAIR(3));
	
	else if(packed->loadavg.min_1 > 0.4)
		attrset(A_BOLD | COLOR_PAIR(2));
	
	printw("% 2.2f ", packed->loadavg.min_1);
	
	
	if(packed->loadavg.min_5 > 1)
		attrset(A_BOLD | COLOR_PAIR(3));
	
	else if(packed->loadavg.min_5 > 0.5)
		attrset(A_BOLD | COLOR_PAIR(2));
	
	printw("% 2.2f ", packed->loadavg.min_5);
	
	if(packed->loadavg.min_15 > 1)
		attrset(A_BOLD | COLOR_PAIR(3));
	
	else if(packed->loadavg.min_15 > 0.5)
		attrset(A_BOLD | COLOR_PAIR(2));
	
	printw("% 2.2f", packed->loadavg.min_15);
	attrset(COLOR_PAIR(1));
	
	printw(" | %s", inet_ntoa(remote->sin_addr));
	
	/* printw(" | " BATTERY_NAME ": %c%d%%", battery_picto[battery.status], battery.load); */
	
	/* Print remote time */
	// printw(" | %02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	
	refresh();
}

void show_packet_network(netinfo_packed_net_t *net, struct sockaddr_in *remote, client_t *client) {
	int i;
	
	remote = NULL;
	move(nbclients + 5 + client->line, 0);
	
	for(i = 0; i < net->nbiface; i++) {
		printw(" %-14s | %-12s |", client->name, net->net[i].name);
		
		if(net->net[i].down_rate > 20000)
			attrset(A_BOLD | COLOR_PAIR(4));
			
		else if(net->net[i].down_rate > 1250)
			attrset(A_BOLD | COLOR_PAIR(3));
		
		else if(net->net[i].down_rate > 100)
			attrset(A_BOLD | COLOR_PAIR(6));
		
		else if(net->net[i].down_rate > 2)
			attrset(A_BOLD | COLOR_PAIR(2));
		
		else attrset(A_BOLD | COLOR_PAIR(5));
		
		printw(" % 15d ko/s", net->net[i].down_rate);
		attrset(COLOR_PAIR(1));
		
		printw(" |");
		if(net->net[i].up_rate > 20000)
			attrset(A_BOLD | COLOR_PAIR(4));
			
		else if(net->net[i].up_rate > 1250)
			attrset(A_BOLD | COLOR_PAIR(3));
		
		else if(net->net[i].up_rate > 100)
			attrset(A_BOLD | COLOR_PAIR(6));
		
		else if(net->net[i].down_rate > 2)
			attrset(A_BOLD | COLOR_PAIR(2));
		
		else attrset(A_BOLD | COLOR_PAIR(5));
		
		printw(" % 15d ko/s", net->net[i].up_rate);
		attrset(COLOR_PAIR(1));
		
		printw(" | %s\n", net->net[i].ip);
	}
	
	// printw("----------------+--------------+------------------------+------------------------");
	
	refresh();
}
