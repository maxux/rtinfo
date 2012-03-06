#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <ncurses.h>
#include "../rtinfo/socket.h"
#include "server.h"
#include "display.h"

extern int nbclients;
char *units[] = {"Ko", "Mo", "Go", "To"};

void refresh_whole() {
	show_header();
	show_net_header();
}

void show_header() {
	move(0, 0);
	
	printw(" Hostname       | CPU Usage                    | RAM            | SWAP         | Load Avg.         | Remote IP       | Time\n");
	printw("----------------+-------+----------------------+----------------+--------------+-------------------+-----------------+----------\n\n");
	
	refresh();
}

void show_net_header() {
	move(nbclients + 2, 0);
	
	printw("\n Hostname       | Interface    | Download Rate        | Download Size | Upload rate          | Upload Size | Interface Address\n");
	printw("----------------+--------------+----------------------+---------------+----------------------+-------------+--------------------");
	
	refresh();
}

double sizeroundd(uint64_t size) {
	unsigned int i;
	double result;
	
	result = size;
	
	for(i = 0; i < sizeof(units) / 2; i++) {
		if(result / 1024 < 1023)
			return result / 1024;
			
		else result /= 1024;
	}
	
	return result;
}

char * unitround(uint64_t size) {
	unsigned int i;
	double result;
	
	result = size;
	
	for(i = 0; i < sizeof(units) / 4; i++) {
		if(result / 1024 < 1023)
			return units[i];
			
		else result /= 1024;
	}
	
	return units[i - 1];
}

void show_packet(netinfo_packed_t *packed, struct sockaddr_in *remote, client_t *client) {
	int i;
	float memory_percent, swap_percent;
	struct tm * timeinfo;
	
	move(client->id + 2, 0);
	
	attrset(COLOR_PAIR(1));
	
	packed->hostname[14] = '\0';
	printw(" %-14s", packed->hostname);
	
	/* Print CPU Usage */
	printw(" | ");
	
	for(i = 0; i < packed->nbcpu; i++) {
		if(packed->cpu[i].usage > 85)
			attrset(A_BOLD | COLOR_PAIR(4));
			
		else if(packed->cpu[i].usage > 50)
			attrset(A_BOLD | COLOR_PAIR(3));
			
		printw("%3d%% ", packed->cpu[i].usage);
		attrset(COLOR_PAIR(1));
		
		if(i == 0)
			printw(" | ");
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
	
	for(i = 0; i < 3; i++) {
		if(packed->loadavg.load[i] > 1)
			attrset(A_BOLD | COLOR_PAIR(3));
		
		else if(packed->loadavg.load[i] > 0.4)
			attrset(A_BOLD | COLOR_PAIR(2));
		
		printw("% 2.2f ", packed->loadavg.load[i]);
		attrset(COLOR_PAIR(1));
	}
	
	printw("| %-15s", inet_ntoa(remote->sin_addr));
	
	/* printw(" | " BATTERY_NAME ": %c%d%%", battery_picto[battery.status], battery.load); */
	
	/* Print remote time */
	timeinfo = localtime((time_t*) &packed->timestamp);
	printw(" | %02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	
	refresh();
}

void show_packet_network(netinfo_packed_net_t *net, struct sockaddr_in *remote, client_t *client) {
	int i, reduce = 0;
	
	remote = NULL;
	move(nbclients + 5 + client->line, 0);
	
	for(i = 0; i < net->nbiface; i++) {
		/* Hide Interfaces without ip and hide loopback interface */
		if(!*(net->net[i].ip) || !strncmp(net->net[i].ip, "127.0.0.1", 9)) {
			reduce++;
			continue;
		}
			
		printw(" %-14s | %-12s |", client->name, net->net[i].name);
		
		if(net->net[i].down_rate > 20 * 1024 * 1024)
			attrset(A_BOLD | COLOR_PAIR(4));
			
		else if(net->net[i].down_rate > 1.5 * 1024 * 1024)
			attrset(A_BOLD | COLOR_PAIR(3));
		
		else if(net->net[i].down_rate > 100 * 1024)
			attrset(A_BOLD | COLOR_PAIR(6));
		
		else if(net->net[i].down_rate > 2 * 1024)
			attrset(A_BOLD | COLOR_PAIR(2));
		
		else attrset(A_BOLD | COLOR_PAIR(5));
		
		printw(" % 15.2f %s/s ", sizeroundd(net->net[i].down_rate), unitround(net->net[i].down_rate));
		attrset(COLOR_PAIR(1));
		printw("| % 10.2f %s", sizeroundd(net->net[i].current.down), unitround(net->net[i].current.down));
		
		printw(" |");
		if(net->net[i].up_rate > 20 * 1024 * 1024)
			attrset(A_BOLD | COLOR_PAIR(4));
			
		else if(net->net[i].up_rate > 1.5 * 1024 * 1024)
			attrset(A_BOLD | COLOR_PAIR(3));
		
		else if(net->net[i].up_rate > 100 * 1024)
			attrset(A_BOLD | COLOR_PAIR(6));
		
		else if(net->net[i].up_rate > 2 * 1024)
			attrset(A_BOLD | COLOR_PAIR(2));
		
		else attrset(A_BOLD | COLOR_PAIR(5));
		
		printw(" % 15.2f %s/s ", sizeroundd(net->net[i].up_rate), unitround(net->net[i].up_rate));
		attrset(COLOR_PAIR(1));
		printw("| % 8.2f %s", sizeroundd(net->net[i].current.up), unitround(net->net[i].current.up));
		
		/* Print IP */
		printw(" | ");
		
		/* Highlight public address */
		if(strncmp(net->net[i].ip, "10.", 3) && strncmp(net->net[i].ip, "172.16.", 7) && strncmp(net->net[i].ip, "192.168.", 7))
			attrset(A_BOLD | COLOR_PAIR(7));
			
		printw("%s\n", net->net[i].ip);
		attrset(COLOR_PAIR(1));
	}
	
	client->nbiface -= reduce;
	printw("----------------+--------------+----------------------+---------------+----------------------+-------------+--------------------");
	
	refresh();
}
