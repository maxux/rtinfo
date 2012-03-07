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

/* Network rate colors */
int rate_limit[] = {
		2   * 1024,		/* 2   Ko/s | Magenta	*/
		100 * 1024,		/* 100 Ko/s | Cyan	*/
		1.5 * 1024 * 1024,	/* 1.5 Mo/s | Yellow	*/
		20  * 1024 * 1024,	/* 20  Mo/s | Red	*/
};

/* Memory (RAM/SWAP) level colors */
int memory_limit[] = {
		30,	/* 30% | White  */
		50,	/* 50% | Yellow */
		85,	/* 85% | Red    */
};

int cpu_limit[] = {
		30,	/* 30% | White  */
		50,	/* 50% | Yellow */
		85,	/* 85% | Red	*/
};

#define RATE_COLD	(A_BOLD | COLOR_PAIR(5))
#define RATE_ACTIVE	(A_BOLD | COLOR_PAIR(8))
#define RATE_LOW	(A_BOLD | COLOR_PAIR(6))
#define RATE_MIDDLE	(A_BOLD | COLOR_PAIR(3))
#define RATE_HIGH	(A_BOLD | COLOR_PAIR(4))

#define LEVEL_COLD	(A_BOLD | COLOR_PAIR(5))
#define LEVEL_ACTIVE	(COLOR_PAIR(1))
#define LEVEL_WARN	(A_BOLD | COLOR_PAIR(3))
#define LEVEL_HIGH	(A_BOLD | COLOR_PAIR(4))

void separe(char wide) {
	int attrs;
	short pair;
	
	/* Saving current state */
	attr_get(&attrs, &pair, NULL);
	
	/* Print sepration */
	attrset(COLOR_PAIR(2));
	if(wide)
		printw(" | ");
		
	else printw("|");
	
	/* Restoring */
	attr_set(attrs, pair, NULL);
}

void title(char *title, int length, char eol) {
	attrset(A_BOLD | COLOR_PAIR(2));
	printw(" %-*s ", length, title);
	
	attrset(COLOR_PAIR(1));
	if(!eol)
		separe(0);
}

void split() {
	attrset(COLOR_PAIR(2));
	hline(ACS_HLINE, 128);
}

void refresh_whole() {
	show_header();
	show_net_header();
}

void show_header() {
	move(0, 0);
	
	title("Hostname", 14, 0);
	title("CPU Usage", 28, 0);
	title("RAM", 14, 0);
	title("SWAP", 12, 0);
	title("Load Avg.", 17, 0);
	title("Remote IP", 15, 0);
	title("Time", 8, 1);
	printw("\n");
	
	split();
	
	refresh();
}

void show_net_header() {
	move(nbclients + 2, 0);
	
	printw("\n");
	
	title("Hostname", 14, 0);
	title("Interface", 12, 0);
	title("Download Rate", 20, 0);
	title("Download Size", 13, 0);
	title("Upload Rate", 20, 0);
	title("Upload Size", 11, 0);
	title("Interface Address", 17, 1);
	printw("\n");
	
	split();	
	refresh();
}

double sizeroundd(uint64_t size) {
	unsigned int i;
	double result = size;
	
	for(i = 0; i < (sizeof(units) / 2) - 1; i++) {
		if(result / 1024 < 1023)
			return result / 1024;
			
		else result /= 1024;
	}
	
	return result;
}

char * unitround(uint64_t size) {
	unsigned int i;
	double result = size / 1024;	/* First unit is Ko */
	
	for(i = 0; i < sizeof(units) / sizeof(units[0]); i++) {
		if(result < 1023)
			break;
			
		result /= 1024;
	}
	
	return units[i];
}

void show_packet(netinfo_packed_t *packed, struct sockaddr_in *remote, client_t *client) {
	int i;
	float memory_percent, swap_percent;
	struct tm * timeinfo;
	
	move(client->id + 2, 0);
	
	attrset(COLOR_PAIR(1));
	
	packed->hostname[14] = '\0';
	attrset(A_BOLD | COLOR_PAIR(7));
	printw(" %-14s", packed->hostname);
	
	/* Print CPU Usage */
	separe(1);
	
	for(i = 0; i < packed->nbcpu; i++) {
		if(packed->cpu[i].usage > 85)
			attrset(LEVEL_HIGH);
			
		else if(packed->cpu[i].usage > 50)
			attrset(LEVEL_WARN);
		
		else if(packed->cpu[i].usage > 30)
			attrset(LEVEL_ACTIVE);
			
		else attrset(LEVEL_COLD);
		
		printw("%3d%% ", packed->cpu[i].usage);
		attrset(COLOR_PAIR(1));
		
		if(i == 0)
			separe(1);
	}
	
	if(packed->nbcpu < 4)
		for(; i < 5; i++)
			printw("     ");
	
	/* Print Memory Usage */
	separe(1);
	memory_percent = ((float) packed->memory.ram_used / packed->memory.ram_total) * 100;
	
	if(memory_percent > memory_limit[2])
		attrset(LEVEL_HIGH);
		
	else if(memory_percent > memory_limit[1])
		attrset(LEVEL_WARN);
	
	else if(memory_percent > memory_limit[0])
		attrset(LEVEL_ACTIVE);
	
	else attrset(LEVEL_COLD);
	
	printw("%5lld Mo (%2.0f%%)", packed->memory.ram_used / 1024, memory_percent);
	attrset(COLOR_PAIR(1));
	
	separe(1);
	if(packed->memory.swap_total > 0) {
		swap_percent = ((float) (packed->memory.swap_total - packed->memory.swap_free) / packed->memory.swap_total) * 100;
		
		if(swap_percent > memory_limit[2])
			attrset(LEVEL_HIGH);
			
		else if(swap_percent > memory_limit[1])
			attrset(LEVEL_WARN);
		
		else if(swap_percent > memory_limit[0])
			attrset(LEVEL_ACTIVE);
		
		else attrset(LEVEL_COLD);
			
		printw("%3lld Mo (%2.0f%%)", (packed->memory.swap_total - packed->memory.swap_free) / 1024, swap_percent);
		
	} else {
		attrset(A_BOLD | COLOR_PAIR(8));	/* Magenta */
		printw("   No swap  ");
	}
	attrset(COLOR_PAIR(1));
	
	/* Print Load Average Usage */
	separe(1);
	
	for(i = 0; i < 3; i++) {
		if(packed->loadavg.load[i] >= packed->nbcpu - 1)
			attrset(LEVEL_HIGH);
			
		else if(packed->loadavg.load[i] > (packed->nbcpu / 2))
			attrset(LEVEL_WARN);
		
		else if(packed->loadavg.load[i] > 0.4)
			attrset(LEVEL_ACTIVE);
		
		else attrset(LEVEL_COLD);
		
		printw("% 2.2f ", packed->loadavg.load[i]);
	}
	
	separe(0);
	
	/* Remote Address */
	attrset(COLOR_PAIR(1));
	printw(" %-15s", inet_ntoa(remote->sin_addr));
	
	/* printw(" | " BATTERY_NAME ": %c%d%%", battery_picto[battery.status], battery.load); */
	
	/* Print remote time */
	separe(1);
	
	timeinfo = localtime((time_t*) &packed->timestamp);
	printw("%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	
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
		
		/* Hostname */
		attrset(A_BOLD | COLOR_PAIR(7));
		printw(" %-14s ", client->name);
		separe(0);
		
		/* Interface */
		attrset(COLOR_PAIR(1));
		printw(" %-12s ", net->net[i].name);
		separe(0);
		
		if(net->net[i].down_rate > rate_limit[3])
			attrset(RATE_HIGH);
			
		else if(net->net[i].down_rate > rate_limit[2])
			attrset(RATE_MIDDLE);
		
		else if(net->net[i].down_rate > rate_limit[1])
			attrset(RATE_LOW);
		
		else if(net->net[i].down_rate > rate_limit[0])
			attrset(RATE_ACTIVE);
		
		else attrset(RATE_COLD);
		
		printw(" % 15.2f %s/s", sizeroundd(net->net[i].down_rate), unitround(net->net[i].down_rate));
		separe(1);
		
		attrset(COLOR_PAIR(1));
		printw("% 10.2f %s", sizeroundd(net->net[i].current.down), unitround(net->net[i].current.down));
		
		separe(1);
		if(net->net[i].up_rate > rate_limit[3])
			attrset(RATE_HIGH);
			
		else if(net->net[i].up_rate > rate_limit[2])
			attrset(RATE_MIDDLE);
		
		else if(net->net[i].up_rate > rate_limit[1])
			attrset(RATE_LOW);
		
		else if(net->net[i].up_rate > rate_limit[0])
			attrset(RATE_ACTIVE);
		
		else attrset(RATE_COLD);
		
		printw("% 15.2f %s/s", sizeroundd(net->net[i].up_rate), unitround(net->net[i].up_rate));
		separe(1);
		
		attrset(COLOR_PAIR(1));
		printw("% 8.2f %s", sizeroundd(net->net[i].current.up), unitround(net->net[i].current.up));
		
		/* Print IP */
		separe(1);
		
		/* Highlight public address */
		if(strncmp(net->net[i].ip, "10.", 3) && strncmp(net->net[i].ip, "172.16.", 7) && strncmp(net->net[i].ip, "192.168.", 7))
			attrset(A_BOLD | COLOR_PAIR(7));
			
		printw("%s\n", net->net[i].ip);
		attrset(COLOR_PAIR(1));
	}
	
	client->nbiface -= reduce;
	split();
	
	clrtoeol();
	refresh();
}
