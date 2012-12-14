#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <ncurses.h>
#include <pthread.h>
#include "../rtinfo-common/socket.h"
#include "server.h"
#include "server_display.h"

extern int nbclients;
char *units[] = {"Ko", "Mo", "Go", "To"};
char *uptime_units[] = {"m", "h", "d"};

pthread_mutex_t mutex_screen;

int x, y;

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

int hdd_limit[] = {
	39,		/* 55°C | Yellow */
	46,		/* 85°C | Red	 */
};

int battery_limit[] = {
	10,		/* 10%  | Red	 */
	20,		/* 20%  | Yellow */
	99,		/* 99%	| Grey	 */
};

char *battery_picto = "=+-?!";
size_t battery_picto_size = 6;

#define RATE_COLD	(A_BOLD | COLOR_PAIR(5))
#define RATE_ACTIVE	(A_BOLD | COLOR_PAIR(8))
#define RATE_LOW	(A_BOLD | COLOR_PAIR(6))
#define RATE_MIDDLE	(A_BOLD | COLOR_PAIR(3))
#define RATE_HIGH	(A_BOLD | COLOR_PAIR(4))

#define LEVEL_COLD	(A_BOLD | COLOR_PAIR(5))
#define LEVEL_ACTIVE	(COLOR_PAIR(1))
#define LEVEL_WARN	(A_BOLD | COLOR_PAIR(3))
#define LEVEL_HIGH	(A_BOLD | COLOR_PAIR(4))

void initdisplay() {
	getmaxyx(stdscr, y, x);
	
	wdebug = newwin(1, WINDOW_WIDTH, y - 2, 0);
	wattrset(wdebug, A_BOLD | COLOR_PAIR(4));
}

void netmoveunder(client_t *root, int offset) {
	// Moving all others...
	while(root) {
		root->nety += offset;
		mvwin(root->netwindow, root->nety, root->netx);
		wrefresh(root->netwindow);
		
		root = root->next;
	}
}

void separe(WINDOW *win, char wide) {
	int attrs;
	short pair;
	
	/* Saving current state */
	wattr_get(win, &attrs, &pair, NULL);
	
	/* Print sepration */
	wattrset(win, COLOR_PAIR(2));
	if(wide)
		wprintw(win, " | ");
		
	else wprintw(win, "|");
	
	/* Restoring */
	wattr_set(win, attrs, pair, NULL);
}

void title(WINDOW *win, char *title, int length, char eol) {
	wattrset(win, A_BOLD | COLOR_PAIR(2));
	wprintw(win, " %-*s ", length, title);
	
	wattrset(win, COLOR_PAIR(1));
	if(!eol)
		separe(win, 0);
}

void split(WINDOW *win) {
	wattrset(win, COLOR_PAIR(2));
	whline(win, ACS_HLINE, WINDOW_WIDTH - 1);
}

void refresh_whole() {
	// show_header();
	// show_net_header();
}

void build_header(WINDOW *win) {
	title(win, "Hostname", 14, 0);
	title(win, "CPU / Nb", 8, 0);
	title(win, "RAM", 14, 0);
	title(win, "SWAP", 14, 0);
	title(win, "Load Avg.", 20, 0);
	title(win, "Remote IP", 15, 0);
	title(win, "Time", 8, 0);
	title(win, "Uptime", 6, 0);
	title(win, "Bat.", 5, 0);
	title(win, "CPU / HDD °C", 10, 1);
	wprintw(win, "\n");
	split(win);
	
	wrefresh(win);
}

void build_netheader(WINDOW *win) {
	title(win, "Hostname", 14, 0);
	title(win, "Interface", 12, 0);
	title(win, "Download Rate", 20, 0);
	title(win, "Download Size", 13, 0);
	title(win, "Upload Rate", 20, 0);
	title(win, "Upload Size", 11, 0);
	title(win, "Interface Address", 17, 0);
	title(win, "Link Speed", 10, 1);
	wprintw(win, "\n");
	
	split(win);
	wrefresh(win);
}

void joining(client_t *client) {
	/* Summary */
	wattrset(client->window, A_BOLD | COLOR_PAIR(5));
	
	wprintw(client->window, " %-14s", client->name);
	separe(client->window, 1);
	wprintw(client->window, "New client, waiting summary data...");
	
	wattrset(client->window, A_BOLD | COLOR_PAIR(1));
	wrefresh(client->window);
	
	/* Network */
	wattrset(client->netwindow, A_BOLD | COLOR_PAIR(5));
	
	wprintw(client->netwindow, " %-14s", client->name);
	separe(client->netwindow, 1);
	wprintw(client->netwindow, "New client, waiting network data...");
	
	wattrset(client->netwindow, A_BOLD | COLOR_PAIR(1));
	wrefresh(client->netwindow);
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

int uptime_value(rtinfo_uptime_t *uptime) {
	if(uptime->uptime < 3600)
		return uptime->uptime / 60;
	
	if(uptime->uptime < 86400)
		return uptime->uptime / 3600;
	
	return uptime->uptime / 86400;
}

char * uptime_unit(rtinfo_uptime_t *uptime) {
	if(uptime->uptime < 3600)
		return uptime_units[0];
	
	if(uptime->uptime < 86400)
		return uptime_units[1];
	
	return uptime_units[2];
}

void show_packet(netinfo_packed_t *packed, struct sockaddr_in *remote, client_t *client) {
	int i;
	float memory_percent, swap_percent;
	struct tm * timeinfo;
	
	/* Locking screen */
	pthread_mutex_lock(&mutex_screen);
	
	wmove(client->window, 0, 0);
	
	wattrset(client->window, COLOR_PAIR(1));
	
	packed->hostname[14] = '\0';
	wattrset(client->window, A_BOLD | COLOR_PAIR(7));
	wprintw(client->window, " %-14s", packed->hostname);
	
	/* Print CPU Usage */
	separe(client->window, 1);
	
	/* Only print Average */
	if(packed->cpu[0].usage > cpu_limit[2])
		wattrset(client->window, LEVEL_HIGH);
		
	else if(packed->cpu[0].usage > cpu_limit[1])
		wattrset(client->window, LEVEL_WARN);
	
	else if(packed->cpu[0].usage > cpu_limit[0])
		wattrset(client->window, LEVEL_ACTIVE);
		
	else wattrset(client->window, LEVEL_COLD);
	
	wprintw(client->window, "%3d%%/% 2d ", packed->cpu[0].usage, packed->nbcpu - 1);
	wattrset(client->window, COLOR_PAIR(1));
	
	
	/* Print Memory Usage */
	separe(client->window, 1);
	memory_percent = ((float) packed->memory.ram_used / packed->memory.ram_total) * 100;
	
	if(memory_percent > memory_limit[2])
		wattrset(client->window, LEVEL_HIGH);
		
	else if(memory_percent > memory_limit[1])
		wattrset(client->window, LEVEL_WARN);
	
	else if(memory_percent > memory_limit[0])
		wattrset(client->window, LEVEL_ACTIVE);
	
	else wattrset(client->window, LEVEL_COLD);
	
	wprintw(client->window, "%5lld Mo (%2.0f%%)", packed->memory.ram_used / 1024, memory_percent);
	wattrset(client->window, COLOR_PAIR(1));
	
	separe(client->window, 1);
	if(packed->memory.swap_total > 0) {
		swap_percent = ((float) (packed->memory.swap_total - packed->memory.swap_free) / packed->memory.swap_total) * 100;
		
		if(swap_percent > memory_limit[2])
			wattrset(client->window, LEVEL_HIGH);
			
		else if(swap_percent > memory_limit[1])
			wattrset(client->window, LEVEL_WARN);
		
		else if(swap_percent > memory_limit[0])
			wattrset(client->window, LEVEL_ACTIVE);
		
		else wattrset(client->window, LEVEL_COLD);
		
		/* FIXME: width fail on %2.0f when full */
		if(swap_percent == 100)
			swap_percent = 99;
			
		wprintw(client->window, "%5lld Mo (%2.0f%%)", (packed->memory.swap_total - packed->memory.swap_free) / 1024, swap_percent);
		
	} else {
		wattrset(client->window, A_BOLD | COLOR_PAIR(8));	/* Magenta */
		wprintw(client->window, "    No swap   ");
	}
	wattrset(client->window, COLOR_PAIR(1));
	
	/* Print Load Average Usage */
	separe(client->window, 1);
	
	for(i = 0; i < 3; i++) {
		if(((float) packed->loadavg[i] / 100) >= packed->nbcpu - 1)
			wattrset(client->window, LEVEL_HIGH);
			
		else if(((float) packed->loadavg[i] / 100) > (packed->nbcpu / 2))
			wattrset(client->window, LEVEL_WARN);
		
		else if(((float) packed->loadavg[i] / 100) > 0.4)
			wattrset(client->window, LEVEL_ACTIVE);
		
		else wattrset(client->window, LEVEL_COLD);
		
		wprintw(client->window, "% 6.2f ", ((float) packed->loadavg[i] / 100));
	}
	
	separe(client->window, 0);
	
	/* Remote Address */
	wattrset(client->window, COLOR_PAIR(1));
	wprintw(client->window, " %-15s", inet_ntoa(remote->sin_addr));
	
	/* wprintw(client->window, " | " BATTERY_NAME ": %c%d%%", battery_picto[battery.status], battery.load); */
	
	/* Print remote time */
	separe(client->window, 1);
	
	timeinfo = localtime((time_t*) &packed->timestamp);
	wprintw(client->window, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	
	/* Print remote uptime */
	separe(client->window, 1);
	wprintw(client->window, "% 4d %s", uptime_value(&packed->uptime), uptime_unit(&packed->uptime));
	
	/* Print remote battery status */
	separe(client->window, 1);
	if(packed->battery.load != -1) {
		if(packed->battery.status != CHARGING) {
			if(packed->battery.load < battery_limit[0])
				wattrset(client->window, LEVEL_HIGH);
				
			else if(packed->battery.load < battery_limit[1])
				wattrset(client->window, LEVEL_WARN);
			
			else if(packed->battery.load < battery_limit[2])
				wattrset(client->window, LEVEL_ACTIVE);
			
			else wattrset(client->window, LEVEL_COLD);
			
		} else wattrset(client->window, RATE_LOW);
		
		if(packed->battery.status < battery_picto_size)
			wprintw(client->window, "%c%3d%%%", battery_picto[packed->battery.status], packed->battery.load);
			
		else wprintw(client->window, "Err. ", packed->battery.load);
		
	} else {
		wattrset(client->window, A_BOLD | COLOR_PAIR(8));	/* Magenta */
		wprintw(client->window, "AC   ");
	}
	
	/* Print remote coretemp value */
	separe(client->window, 1);
	
	if(!packed->temp_cpu.critical)
		packed->temp_cpu.critical = 100;
		
	if(packed->temp_cpu.cpu_average > 0) {
		if(packed->temp_cpu.cpu_average > packed->temp_cpu.critical * 0.8)
			wattrset(client->window, LEVEL_HIGH);
		
		else if(packed->temp_cpu.cpu_average > packed->temp_cpu.critical * 0.6)
			wattrset(client->window, LEVEL_WARN);
			
		else wattrset(client->window, LEVEL_COLD);
		
		wprintw(client->window, "% 3d", packed->temp_cpu.cpu_average);
		
	} else wprintw(client->window, "   ");
	
	wattrset(client->window, LEVEL_COLD);
	wprintw(client->window, " / ");
	
	if(packed->temp_hdd.hdd_average > 0) {
		if(packed->temp_hdd.hdd_average > hdd_limit[1])
			wattrset(client->window, LEVEL_HIGH);
		
		else if(packed->temp_hdd.hdd_average > hdd_limit[0])
			wattrset(client->window, LEVEL_WARN);
			
		else wattrset(client->window, LEVEL_COLD);
		
		wprintw(client->window, "% 2d ", packed->temp_hdd.hdd_average);
	}
	
	if(packed->temp_hdd.peak > 0) {
		if(packed->temp_hdd.peak > hdd_limit[1])
			wattrset(client->window, LEVEL_HIGH);
		
		else if(packed->temp_hdd.peak > hdd_limit[0])
			wattrset(client->window, LEVEL_WARN);
			
		else wattrset(client->window, LEVEL_COLD);
		
		wprintw(client->window, "(%d)", packed->temp_hdd.peak);
		
	} else wprintw(client->window, "    ");
	
	/* End of line */
	wclrtoeol(client->window);
	wrefresh(client->window);
	
	/* Unlocking screen */
	pthread_mutex_unlock(&mutex_screen);
}

void show_packet_network(netinfo_packed_net_t *net, client_t *client) {
	uint32_t i;
	int reduce = 0;
	
	/* Console too small */
	/* if(nbclients + 5 + client->line + client->nbiface > y)
		return; */
	
	/* Locking screen */
	pthread_mutex_lock(&mutex_screen);
	
	wmove(client->netwindow, 0, 0);
	
	// Grow/Shrink window
	if(net->nbiface != client->nbiface)
		wresize(client->netwindow, net->nbiface + 1, WINDOW_WIDTH);
	
	for(i = 0; i < net->nbiface; i++) {
		/* Hide Interfaces without ip and hide loopback interface */
		if((!*(net->net[i].ip) && !net->net[i].speed) || !strncmp(net->net[i].ip, "127.0.0.1", 9)) {
			reduce++;
			continue;
		}
	
		/* Hostname */
		wattrset(client->netwindow, A_BOLD | COLOR_PAIR(7));
		wprintw(client->netwindow, " %-14s ", client->name);
		separe(client->netwindow, 0);
		
		/* Interface */
		wattrset(client->netwindow, COLOR_PAIR(1));
		wprintw(client->netwindow, " %-12s ", net->net[i].name);
		separe(client->netwindow, 0);
		
		if(net->net[i].down_rate > rate_limit[3])
			wattrset(client->netwindow, RATE_HIGH);
			
		else if(net->net[i].down_rate > rate_limit[2])
			wattrset(client->netwindow, RATE_MIDDLE);
		
		else if(net->net[i].down_rate > rate_limit[1])
			wattrset(client->netwindow, RATE_LOW);
		
		else if(net->net[i].down_rate > rate_limit[0])
			wattrset(client->netwindow, RATE_ACTIVE);
		
		else wattrset(client->netwindow, RATE_COLD);
		
		wprintw(client->netwindow, " % 15.2f %s/s", sizeroundd(net->net[i].down_rate), unitround(net->net[i].down_rate));
		separe(client->netwindow, 1);
		
		wattrset(client->netwindow, COLOR_PAIR(1));
		wprintw(client->netwindow, "% 10.2f %s", sizeroundd(net->net[i].current.down), unitround(net->net[i].current.down));
		
		separe(client->netwindow, 1);
		if(net->net[i].up_rate > rate_limit[3])
			wattrset(client->netwindow, RATE_HIGH);
			
		else if(net->net[i].up_rate > rate_limit[2])
			wattrset(client->netwindow, RATE_MIDDLE);
		
		else if(net->net[i].up_rate > rate_limit[1])
			wattrset(client->netwindow, RATE_LOW);
		
		else if(net->net[i].up_rate > rate_limit[0])
			wattrset(client->netwindow, RATE_ACTIVE);
		
		else wattrset(client->netwindow, RATE_COLD);
		
		wprintw(client->netwindow, "% 15.2f %s/s", sizeroundd(net->net[i].up_rate), unitround(net->net[i].up_rate));
		separe(client->netwindow, 1);
		
		wattrset(client->netwindow, COLOR_PAIR(1));
		wprintw(client->netwindow, "% 8.2f %s", sizeroundd(net->net[i].current.up), unitround(net->net[i].current.up));
		
		/* Print IP */
		separe(client->netwindow, 1);
		
		/* Highlight public address */
		if(strncmp(net->net[i].ip, "10.", 3) && strncmp(net->net[i].ip, "172.16.", 7) && strncmp(net->net[i].ip, "192.168.", 7))
			wattrset(client->netwindow, A_BOLD | COLOR_PAIR(7));
			
		wprintw(client->netwindow, "%-16s ", net->net[i].ip);
		wattrset(client->netwindow, COLOR_PAIR(1));
		
		separe(client->netwindow, 1);
		
		/* Print Speed */
		if(!net->net[i].speed) {
			wattrset(client->netwindow, RATE_COLD);
			wprintw(client->netwindow, "Unknown\n");
			
		} else wprintw(client->netwindow, "%d Mbps\n", net->net[i].speed);
	}
	
	split(client->netwindow);
	wclrtoeol(client->netwindow);
	
	// Shrink if necessary
	if(net->nbiface - reduce != client->nbiface) {
		wresize(client->netwindow, net->nbiface + 1, WINDOW_WIDTH);
		
		// Redrawing all networks below
		netmoveunder(client->next, net->nbiface - reduce - client->nbiface);
		
		// Saving new value
		client->nbiface = net->nbiface - reduce;
	}
	
	wrefresh(client->netwindow);	
	
	/* Unlocking screen */
	pthread_mutex_unlock(&mutex_screen);
}
