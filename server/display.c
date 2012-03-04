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

void show_packet(netinfo_packed_t *packed, struct sockaddr_in *remote, client_t *client) {
	int i;
	float memory_percent, swap_percent;
	
	move(client->id, 0);
	
	attrset(COLOR_PAIR(1));
	
	/* Print Hostname */
	printw(" %-10s", packed->hostname);
	
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
	
	else if(packed->loadavg.min_1 > 0.1)
		attrset(COLOR_PAIR(2));
	
	printw("%.2f ", packed->loadavg.min_1);
	
	
	if(packed->loadavg.min_5 > 1)
		attrset(A_BOLD | COLOR_PAIR(3));
	
	else if(packed->loadavg.min_5 > 0.1)
		attrset(COLOR_PAIR(2));
	
	printw("%.2f", packed->loadavg.min_5);
	attrset(COLOR_PAIR(1));
	
	/* Print remote IP Address */
	printw(" | %-16s", inet_ntoa(remote->sin_addr));
	
	
	/* printw(" | " BATTERY_NAME ": %c%d%%", battery_picto[battery.status], battery.load); */
	
	/* Print remote time */
	// printw(" | %02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	
	refresh();
}
