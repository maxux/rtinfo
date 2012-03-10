#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <ncurses.h>
#include <rtinfo.h>
#include <inttypes.h>
#include "rtinfo-client.h"
#include "rtinfo-local.h"

#define RATE_COLD	(A_BOLD | COLOR_PAIR(5))
#define RATE_ACTIVE	(A_BOLD | COLOR_PAIR(8))
#define RATE_LOW	(A_BOLD | COLOR_PAIR(6))
#define RATE_MIDDLE	(A_BOLD | COLOR_PAIR(3))
#define RATE_HIGH	(A_BOLD | COLOR_PAIR(4))

#define LEVEL_COLD	(A_BOLD | COLOR_PAIR(5))
#define LEVEL_ACTIVE	(COLOR_PAIR(1))
#define LEVEL_WARN	(A_BOLD | COLOR_PAIR(3))
#define LEVEL_HIGH	(A_BOLD | COLOR_PAIR(4))

#define TITLE_STYLE	(A_BOLD | COLOR_PAIR(6))

int x, y;
char *units[] = {"Ko", "Mo", "Go", "To"};
char *uptime_units[] = {"min", "hrs", "days"};

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

void dummy(int signal) {
	switch(signal) {
		case SIGINT:
			endwin();
			exit(0);
		break;
		
		case SIGWINCH:
			endwin();
			clear();
			refresh();
			
			getmaxyx(stdscr, y, x);
			
			move(0, 0);
			printw("Reloading...");
			refresh();
			
			usleep(200000);
		break;
	}
}

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

void split() {
	attrset(COLOR_PAIR(6));
	hline(ACS_HLINE, x - 2);
}

int localside() {
	rtinfo_memory_t memory;
	rtinfo_loadagv_t loadavg;
	rtinfo_cpu_t *cpu;
	
	rtinfo_uptime_t uptime;
	
	rtinfo_battery_t battery;
	int use_battery = 0;
	char *battery_picto = "=+-";
	
	rtinfo_network_t *net;
	float memory_percent, swap_percent;
	
	int nbcpu, nbiface, i;
	struct tm * timeinfo;
	char hostname[32];
	
	
	/* Reading hostname */
	if(gethostname(hostname, sizeof(hostname)))
		diep("gethostname");
	
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
	
	/* Initializing variables */
	net = rtinfo_init_network(&nbiface);
	cpu = rtinfo_init_cpu(&nbcpu);
	
	/* Loading curses */
	getmaxyx(stdscr, y, x);
	
	printw("Loading...");
	refresh();
		
	/* Working */	
	while(1) {	
		/* Pre-reading data */
		rtinfo_get_cpu(cpu, nbcpu);
		rtinfo_get_network(net, nbiface);

		/* Sleeping */
		usleep(UPDATE_INTERVAL);
		
		/* Reading CPU */
		rtinfo_get_cpu(cpu, nbcpu);
		rtinfo_mk_cpu_usage(cpu, nbcpu);
		
		/* Reading Network */
		rtinfo_get_network(net, nbiface);
		rtinfo_mk_network_usage(net, nbiface, UPDATE_INTERVAL / 1000);
		
		/* Reading Memory */
		if(!rtinfo_get_memory(&memory))
			return 1;
		
		/* Reading Load Average */
		if(!rtinfo_get_loadavg(&loadavg))
			return 1;
		
		/* Reading Battery State */
		if(use_battery && !rtinfo_get_battery(&battery))
			return 1;
		
		if(!rtinfo_get_uptime(&uptime))
			return 1;
		
		/* Reading Time Info */
		timeinfo = rtinfo_get_time();
		
		/*
		 * ncurses data printing
		 */
		
		move(0, 0);
	
		/* Display Hostname */
		attrset(COLOR_PAIR(1));
		printw(" > ");
		
		attrset(A_BOLD | COLOR_PAIR(7));
		printw("%s ", hostname);
		
		/* Uptime */
		attrset(COLOR_PAIR(8));
		printw("(Uptime: %d %s)", uptime_value(&uptime), uptime_unit(&uptime));
		
		/* Time */
		move(0, x - 10);
		attrset(COLOR_PAIR(1));
		printw("%02d:%02d:%02d\n\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		
		/* Print CPU Usage */
		// split();
		attrset(TITLE_STYLE);
		printw("\n CPU  : ");
		
		for(i = 0; i < nbcpu; i++) {
			if(cpu[i].usage > 85)
				attrset(LEVEL_HIGH);
				
			else if(cpu[i].usage > 50)
				attrset(LEVEL_WARN);
			
			else if(cpu[i].usage > 30)
				attrset(LEVEL_ACTIVE);
				
			else attrset(LEVEL_COLD);
			
			printw("%3d%% ", cpu[i].usage);
			attrset(COLOR_PAIR(1));
			
			if(i == 0)
				separe(1);
		}
		
		/* Print Memory Usage */
		attrset(TITLE_STYLE);
		printw("\n\n RAM  : ");
		
		memory_percent = ((float) memory.ram_used / memory.ram_total) * 100;
		
		if(memory_percent > memory_limit[2])
			attrset(LEVEL_HIGH);
			
		else if(memory_percent > memory_limit[1])
			attrset(LEVEL_WARN);
		
		else if(memory_percent > memory_limit[0])
			attrset(LEVEL_ACTIVE);
		
		else attrset(LEVEL_COLD);
		
		printw("% 6lld Mo (%2.0f%%)", memory.ram_used / 1024, memory_percent);
		
		attrset(TITLE_STYLE);
		printw("\n SWAP : ");
		if(memory.swap_total > 0) {
			swap_percent = ((float) (memory.swap_total - memory.swap_free) / memory.swap_total) * 100;
			
			if(swap_percent > memory_limit[2])
				attrset(LEVEL_HIGH);
				
			else if(swap_percent > memory_limit[1])
				attrset(LEVEL_WARN);
			
			else if(swap_percent > memory_limit[0])
				attrset(LEVEL_ACTIVE);
			
			else attrset(LEVEL_COLD);
				
			printw("% 6lld Mo (%2.0f%%)", (memory.swap_total - memory.swap_free) / 1024, swap_percent);
			
		} else {
			attrset(A_BOLD | COLOR_PAIR(8));	/* Magenta */
			printw("No swap");
		}
		
		
		/* Print Load Average Usage */
		attrset(TITLE_STYLE);
		printw("\n\n Load : ");
		
		for(i = 0; i < 3; i++) {
			if(loadavg.load[i] >= nbcpu - 1)
				attrset(LEVEL_HIGH);
				
			else if(loadavg.load[i] > (nbcpu / 2))
				attrset(LEVEL_WARN);
			
			else if(loadavg.load[i] > 0.4)
				attrset(LEVEL_ACTIVE);
			
			else attrset(LEVEL_COLD);
			
			printw("% 2.2f ", loadavg.load[i]);
		}
		
		
		/* Battery */
		move(3, (x / 2) - 4);
		separe(1);
		
		attrset(TITLE_STYLE);
		printw("Battery    : ");
		
		if(use_battery) {
			if(battery.load < 10)
				attrset(LEVEL_HIGH);
				
			else if(battery.load < 50)
				attrset(LEVEL_WARN);
				
			else if(battery.load < 80)
				attrset(LEVEL_ACTIVE);
				
			else attrset(LEVEL_COLD);
			
			printw("%c%d%% (" BATTERY_NAME ")", battery_picto[battery.status], battery.load);
		} else {
			attrset(LEVEL_COLD);
			printw("none");
		}
		
		
		/* Full RAM */
		move(4, (x / 2) - 4);
		separe(1);
		
		move(5, (x / 2) - 4);
		separe(1);
		
		attrset(TITLE_STYLE);
		printw("RAM Total  : ");
		
		attrset(LEVEL_COLD);
		printw("% 6lld Mo", memory.ram_total / 1024);
		
		
		/* Full SWAP */
		move(6, (x / 2) - 4);
		separe(1);
		
		attrset(TITLE_STYLE);
		printw("SWAP Total : ");
		
		attrset(LEVEL_COLD);
		printw("% 6lld Mo", memory.swap_total / 1024);
		
		/* Network Interfaces */
		move(7, (x / 2) - 4);
		separe(1);
		
		move(8, (x / 2) - 4);
		separe(1);
		
		attrset(TITLE_STYLE);
		printw("Net Inter. : ");
		
		attrset(LEVEL_COLD);
		printw("%d", nbiface);
		
		/*
		 * Network part
		 */
		move(10, 0);
		
		for(i = 0; i < nbiface; i++) {
			/* Interface */
			attrset(COLOR_PAIR(1));
			printw(" %-12s", net[i].name);
			separe(0);
			
			if(net[i].down_rate > rate_limit[3])
				attrset(RATE_HIGH);
				
			else if(net[i].down_rate > rate_limit[2])
				attrset(RATE_MIDDLE);
			
			else if(net[i].down_rate > rate_limit[1])
				attrset(RATE_LOW);
			
			else if(net[i].down_rate > rate_limit[0])
				attrset(RATE_ACTIVE);
			
			else attrset(RATE_COLD);
			
			printw(" % *.2f %s/s", (x / 2) - 38, sizeroundd(net[i].down_rate), unitround(net[i].down_rate));
			separe(1);
			
			attrset(COLOR_PAIR(1));
			printw("% 8.2f %s\n", sizeroundd(net[i].current.down), unitround(net[i].current.down));
		}
		
		for(i = 0; i < nbiface; i++) {
			move(10 + i, (x / 2) - 4);
			separe(1);
			
			if(net[i].up_rate > rate_limit[3])
				attrset(RATE_HIGH);
				
			else if(net[i].up_rate > rate_limit[2])
				attrset(RATE_MIDDLE);
			
			else if(net[i].up_rate > rate_limit[1])
				attrset(RATE_LOW);
			
			else if(net[i].up_rate > rate_limit[0])
				attrset(RATE_ACTIVE);
			
			else attrset(RATE_COLD);
			
			printw("% *.2f %s/s", (x / 2) - 38, sizeroundd(net[i].up_rate), unitround(net[i].up_rate));
			separe(1);
			
			attrset(COLOR_PAIR(1));
			printw("% 8.2f %s", sizeroundd(net[i].current.up), unitround(net[i].current.up));
			
			/* Print IP */
			separe(1);
			printw("%s\n", net[i].ip);
			attrset(COLOR_PAIR(1));
		}
		
		refresh();
	}
	
	return 0;
}
