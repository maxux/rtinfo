/*
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ncurses.h>
#include <jansson.h>
#include <locale.h>
#include <time.h>
#include <errno.h>
#include "rtinfo_ncurses.h"
#include "rtinfo_client_data.h"
#include "rtinfo_display.h"
#include "rtinfo_socket.h"
#include "rtinfo_units.h"

int __maxy, __maxx;
WINDOW *root_window, *wdebug;

/* Network rate colors */
unsigned int rate_limit[] = {
	2   * 1024,		/* 2   Ko/s | Magenta	*/
	100 * 1024,		/* 100 Ko/s | Cyan	*/
	1.5 * 1024 * 1024,	/* 1.5 Mo/s | Yellow	*/
	20  * 1024 * 1024,	/* 20  Mo/s | Red	*/
};

unsigned int disk_limit[] = {
	10,  /* 10 MiB/s  | White  */
	80,  /* 80 MiB/s  | Yellow */
	150, /* 150 MiB/s | Red    */
};

unsigned int iops_limit[] = {
	10,  /* 10  | White  */
	100, /* 100 | Yellow */
	300, /* 300 | Red    */
};

/* Memory (RAM/SWAP) level colors */
unsigned int memory_limit[] = {
	30,	/* 30% | White  */
	50,	/* 50% | Yellow */
	85,	/* 85% | Red    */
};

/* CPU Usage level colors */
unsigned int cpu_limit[] = {
	30,	/* 30% | White  */
	50,	/* 50% | Yellow */
	85,	/* 85% | Red	*/
};

/* HDD sensors level colors */
unsigned int hdd_limit[] = {
	39,		/* 55°C | Yellow */
	46,		/* 85°C | Red	 */
};

/* Battery level colors */
int battery_limit[] = {
	10,		/* 10%  | Red	 */
	20,		/* 20%  | Yellow */
	99,		/* 99%	| Grey	 */
};

char battery_picto[] = "=+-?!";

int network_skip = 0;
int network_skpped = 0;
int network_displayed = 0;
int network_maxdisplay = 0;

#define RATE_COLD	(A_BOLD | COLOR_PAIR(5))
#define RATE_ACTIVE	(A_BOLD | COLOR_PAIR(8))
#define RATE_LOW	(A_BOLD | COLOR_PAIR(6))
#define RATE_MIDDLE	(A_BOLD | COLOR_PAIR(3))
#define RATE_HIGH	(A_BOLD | COLOR_PAIR(4))

#define LEVEL_COLD	(A_BOLD | COLOR_PAIR(5))
#define LEVEL_ACTIVE	(COLOR_PAIR(1))
#define LEVEL_WARN	(A_BOLD | COLOR_PAIR(3))
#define LEVEL_HIGH	(A_BOLD | COLOR_PAIR(4))

void initconsole() {
	/* Init Console */
	initscr();		/* Init ncurses */
	cbreak();		/* No break line */
	noecho();		/* No echo key */
	start_color();		/* Enable color */
	use_default_colors();
	curs_set(0);		/* Disable cursor */
	keypad(stdscr, TRUE);
	scrollok(stdscr, 1);
	timeout(1000);
	setlocale(LC_CTYPE, "");

	getmaxyx(stdscr, __maxy, __maxx);

	/* Init Colors */
	init_pair(1, COLOR_WHITE,   -1);
	init_pair(2, COLOR_BLUE,    -1);
	init_pair(3, COLOR_YELLOW,  -1);
	init_pair(4, COLOR_RED,     -1);
	init_pair(5, COLOR_BLACK,   -1);
	init_pair(6, COLOR_CYAN,    -1);
	init_pair(7, COLOR_GREEN,   -1);
	init_pair(8, COLOR_MAGENTA, -1);
	init_pair(9, COLOR_WHITE,   -1);
	refresh();

	/* root window full screen */
	root_window = newwin(__maxy - 1, __maxx, 0, 0);
	wdebug      = newwin(1, __maxx, __maxy - 1, 0);
}

void display_perror(char *str) {
	// perror(str);
	wattrset(wdebug, A_BOLD | COLOR_PAIR(4));
	wmove(wdebug, 0, 0);
	wprintw(wdebug, "[-] %s: %s", str, strerror(errno));
	wclrtoeol(root_window);
	wrefresh(wdebug);
}

void display_error(char *str) {
	// printf("%s\n", str);
	wattrset(wdebug, A_BOLD | COLOR_PAIR(4));
	wmove(wdebug, 0, 0);
	wprintw(wdebug, "[-] %s", str);
	wclrtoeol(wdebug);
	wrefresh(wdebug);
}

void display_clrerror() {
	wmove(wdebug, 0, 0);
	wclrtoeol(wdebug);
	wrefresh(wdebug);
}

void separe(WINDOW *win) {
	attr_t attrs;
	int x, y;
	short pair;

	/* Saving current state */
	wattr_get(win, &attrs, &pair, NULL);

	/* Print sepration */
	wattrset(win, COLOR_PAIR(2));
	wvline(win, ACS_VLINE, 1);

	getyx(win, y, x);
	wmove(win, y, x + 1);

	/* Restoring */
	wattr_set(win, attrs, pair, NULL);
}

void title(WINDOW *win, char *title, int length, char eol) {
	wattrset(win, A_BOLD | COLOR_PAIR(2));
	wprintw(win, " %-*s ", length, title);

	wattrset(win, COLOR_PAIR(1));
	if(!eol)
		separe(win);
}

void split(WINDOW *win) {
	int x, y;
	wattrset(win, COLOR_PAIR(2));
	whline(win, ACS_HLINE, WINDOW_WIDTH - 1);
	getyx(win, y, x);
	wmove(win, y + 1, x);
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
	title(win, "CPU / HDD C", 12, 0);
	title(win, "I/O MiB/s", 9, 0);
	// title(win, "IOPS", 10, 1);
	wprintw(win, "\n");

	split(win);
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
}

void erase_anythingelse(WINDOW *win) {
	int x, y;
	getyx(win, y, x);
	(void) x;

	for(; y < __maxy; y++)
		wclrtoeol(root_window);
}

// TODO: rewrite this...
void print_client_summary(client_data_t *client) {
	int i;
	float memory_percent, swap_percent;
	struct tm * timeinfo;
	char buffer[64], *format;
	uint8_t batload = 0;

	if(client->lasttime < time(NULL) - 30)
		wattrset(root_window, A_BOLD | COLOR_PAIR(4));

	else if(client->lasttime < time(NULL) - 5)
		wattrset(root_window, A_BOLD | COLOR_PAIR(3));

	else wattrset(root_window, A_BOLD | COLOR_PAIR(7));

	snprintf(buffer, 15, "%s", client->hostname);
	wprintw(root_window, " %-14s ", buffer);

	/* Print CPU Usage */
	separe(root_window);

	/* Print cpu usage average */
	if(client->summary.cpu_usage > cpu_limit[2])
		wattrset(root_window, LEVEL_HIGH);

	else if(client->summary.cpu_usage > cpu_limit[1])
		wattrset(root_window, LEVEL_WARN);

	else if(client->summary.cpu_usage > cpu_limit[0])
		wattrset(root_window, LEVEL_ACTIVE);

	else wattrset(root_window, LEVEL_COLD);

	wprintw(root_window, " %3ld%%/%3ld ", client->summary.cpu_usage, client->summary.cpu_count);
	wattrset(root_window, COLOR_PAIR(1));


	/* Print Memory Usage */
	separe(root_window);
	memory_percent = ((float) client->summary.ram_used / client->summary.ram_total) * 100;

	if(memory_percent > memory_limit[2])
		wattrset(root_window, LEVEL_HIGH);

	else if(memory_percent > memory_limit[1])
		wattrset(root_window, LEVEL_WARN);

	else if(memory_percent > memory_limit[0])
		wattrset(root_window, LEVEL_ACTIVE);

	else wattrset(root_window, LEVEL_COLD);

	format = (memory_percent > 99.0) ? "%6lld %s (%3.0f%%)" : "%6lld %s (%2.0f%%) ";
	wprintw(root_window, format, client->summary.ram_used / 1024, units_bytes[1], memory_percent);

	wattrset(root_window, COLOR_PAIR(1));

	separe(root_window);
	if(client->summary.swap_total > 0) {
		swap_percent = ((float) (client->summary.swap_total - client->summary.swap_free) / client->summary.swap_total) * 100;

		if(swap_percent > memory_limit[2])
			wattrset(root_window, LEVEL_HIGH);

		else if(swap_percent > memory_limit[1])
			wattrset(root_window, LEVEL_WARN);

		else if(swap_percent > memory_limit[0])
			wattrset(root_window, LEVEL_ACTIVE);

		else wattrset(root_window, LEVEL_COLD);

		format = (swap_percent > 99.0) ? "%6lld %s (%3.0f%%)" : "%6lld %s (%2.0f%%) ";
		wprintw(root_window, format, (client->summary.swap_total - client->summary.swap_free) / 1024, units_bytes[1], swap_percent);

	} else {
		wattrset(root_window, A_BOLD | COLOR_PAIR(8));	/* Magenta */
		wprintw(root_window, "    No swap     ");
	}
	wattrset(root_window, COLOR_PAIR(1));

	/* Print Load Average Usage */
	separe(root_window);

	for(i = 0; i < 3; i++) {
		if(client->summary.loadavg[i] >= client->summary.cpu_count)
			wattrset(root_window, LEVEL_HIGH);

		else if(client->summary.loadavg[i] > ((client->summary.cpu_count + 1) / 2))
			wattrset(root_window, LEVEL_WARN);

		else if(client->summary.loadavg[i] > 0.42)
			wattrset(root_window, LEVEL_ACTIVE);

		else wattrset(root_window, LEVEL_COLD);

		wprintw(root_window, "% 6.2f ", client->summary.loadavg[i]);
	}

	wprintw(root_window, " ");
	separe(root_window);

	/* Remote Address */
	wattrset(root_window, COLOR_PAIR(1));
	wprintw(root_window, " %-15s ", client->remoteip);

	/* Print remote time */
	separe(root_window);

	timeinfo = localtime((time_t*) &client->summary.time);
	wprintw(root_window, " %02d:%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	/* Print remote uptime */
	separe(root_window);
	wprintw(root_window, "% 5d %s ", uptime_value(client->summary.uptime), uptime_unit(client->summary.uptime));

	/* Print remote battery status */
	separe(root_window);
	if(client->summary.battery_load != -1) {
		batload = (uint8_t) client->summary.battery_load;

		/* Fix battery load when kernel set it to upper 100% value */
		if(batload > 100)
			batload = 100;

		if(client->summary.battery_status != BATTERY_CHARGING) {
			if(batload < battery_limit[0])
				wattrset(root_window, LEVEL_HIGH);

			else if(batload < battery_limit[1])
				wattrset(root_window, LEVEL_WARN);

			else if(batload < battery_limit[2])
				wattrset(root_window, LEVEL_ACTIVE);

			else wattrset(root_window, LEVEL_COLD);

		} else wattrset(root_window, RATE_LOW);

		if(client->summary.battery_status < sizeof(battery_picto))
			wprintw(root_window, " %c%3d%%  ", battery_picto[client->summary.battery_status], batload);

		else wprintw(root_window, " Err.  ");

	} else {
		wattrset(root_window, A_BOLD | COLOR_PAIR(8));	/* Magenta */
		wprintw(root_window, "  AC  ");
	}

	/* Print remote coretemp value */
	wprintw(root_window, " ");
	separe(root_window);

	if(!client->summary.sensors_cpu_crit)
		client->summary.sensors_cpu_crit = 100;

	if(client->summary.sensors_cpu_avg > 0) {
		if(client->summary.sensors_cpu_avg > client->summary.sensors_cpu_crit * 0.8)
			wattrset(root_window, LEVEL_HIGH);

		else if(client->summary.sensors_cpu_avg > client->summary.sensors_cpu_crit * 0.6)
			wattrset(root_window, LEVEL_WARN);

		else wattrset(root_window, LEVEL_COLD);

		wprintw(root_window, "% 3ld", client->summary.sensors_cpu_avg);

	} else {
		wattrset(root_window, LEVEL_COLD);
		wprintw(root_window, " --");
	}

	wattrset(root_window, LEVEL_COLD);
	wprintw(root_window, " / ");

	if(client->summary.sensors_hdd_avg > 0) {
		if(client->summary.sensors_hdd_avg > hdd_limit[1])
			wattrset(root_window, LEVEL_HIGH);

		else if(client->summary.sensors_hdd_avg > hdd_limit[0])
			wattrset(root_window, LEVEL_WARN);

		else wattrset(root_window, LEVEL_COLD);

		wprintw(root_window, "%02ld ", client->summary.sensors_hdd_avg);
	}

	if(client->summary.sensors_hdd_peak > 0) {
		if(client->summary.sensors_hdd_peak > hdd_limit[1])
			wattrset(root_window, LEVEL_HIGH);

		else if(client->summary.sensors_hdd_peak > hdd_limit[0])
			wattrset(root_window, LEVEL_WARN);

		else wattrset(root_window, LEVEL_COLD);

		wprintw(root_window, "(%ld) ", client->summary.sensors_hdd_peak);

	} else {
		wattrset(root_window, LEVEL_COLD);
		wprintw(root_window, "--      ");
	}

	separe(root_window);

	if(client->diskcount > 0) {
		double speed = 0;
		// long iops = 0;

		for(i = 0; i < (int) client->diskcount; i++) {
			speed += (client->disk[i].read_speed + client->disk[i].write_speed) / (1024 * 1024.0);
			// iops  += client->disk[i].iops;
		}

		if(speed > disk_limit[2])
			wattrset(root_window, LEVEL_HIGH);

		else if(speed > disk_limit[1])
			wattrset(root_window, LEVEL_WARN);

		else if(speed > disk_limit[0])
			wattrset(root_window, LEVEL_ACTIVE);

		else wattrset(root_window, LEVEL_COLD);

		wprintw(root_window, " % 9.1f ", speed);

		/*
		separe(root_window);

		if(iops > iops_limit[2])
			wattrset(root_window, LEVEL_HIGH);

		else if(iops > iops_limit[1])
			wattrset(root_window, LEVEL_WARN);

		else if(iops > iops_limit[0])
			wattrset(root_window, LEVEL_ACTIVE);

		else wattrset(root_window, LEVEL_COLD);

		wprintw(root_window, " % 7ld", iops);
		*/

	} else wprintw(root_window, "        -- ");

	separe(root_window);

	/* End of line */
	wclrtoeol(root_window);
	wprintw(root_window, "\n");
}

void print_client_network(client_data_t *client, int units) {
	size_t i, j;
	char buffer[64];

	unsigned long long rxr, txr, rxd, txd;

	for(i = 0, j = 0; i < client->ifcount; i++) {
		/* Hide interfaces without ip and hide loopback interface */
		if((!strcmp(client->network[i].ip, "0.0.0.0") || !strcmp(client->network[i].ip, "127.0.0.1")))
			continue;

		/* Network scroll */
		if(network_skpped < network_skip) {
			network_skpped++;
			continue;
		}

		if(network_displayed >= network_maxdisplay)
			continue;

		/* Hostname */
		if(client->lasttime < time(NULL) - 30)
			wattrset(root_window, A_BOLD | COLOR_PAIR(4));

		else if(client->lasttime < time(NULL) - 5)
			wattrset(root_window, A_BOLD | COLOR_PAIR(3));

		else wattrset(root_window, A_BOLD | COLOR_PAIR(7));

		snprintf(buffer, 15, "%s", client->hostname);
		wprintw(root_window, " %-14s ", buffer);

		separe(root_window);

		/* Interface */
		wattrset(root_window, COLOR_PAIR(1));
		wprintw(root_window, " %-12s ", client->network[i].ifname);
		separe(root_window);

		if(client->network[i].rx_rate > rate_limit[3])
			wattrset(root_window, RATE_HIGH);

		else if(client->network[i].rx_rate > rate_limit[2])
			wattrset(root_window, RATE_MIDDLE);

		else if(client->network[i].rx_rate > rate_limit[1])
			wattrset(root_window, RATE_LOW);

		else if(client->network[i].rx_rate > rate_limit[0])
			wattrset(root_window, RATE_ACTIVE);

		else wattrset(root_window, RATE_COLD);

		rxr = client->network[i].rx_rate;
		txr = client->network[i].tx_rate;
		rxd = client->network[i].rx_data;
		txd = client->network[i].tx_data;

		wprintw(root_window, " % 15.2f %s/s ", sizeroundd(rxr, units), unitround(rxr, units));
		separe(root_window);

		wattrset(root_window, COLOR_PAIR(1));
		wprintw(root_window, "% 11.2f %s ", sizeroundd(rxd, units), unitround(rxd, units));

		separe(root_window);
		if(client->network[i].tx_rate > rate_limit[3])
			wattrset(root_window, RATE_HIGH);

		else if(client->network[i].tx_rate > rate_limit[2])
			wattrset(root_window, RATE_MIDDLE);

		else if(client->network[i].tx_rate > rate_limit[1])
			wattrset(root_window, RATE_LOW);

		else if(client->network[i].tx_rate > rate_limit[0])
			wattrset(root_window, RATE_ACTIVE);

		else wattrset(root_window, RATE_COLD);

		wprintw(root_window, " % 15.2f %s/s ", sizeroundd(txr, units), unitround(txr, units));
		separe(root_window);

		wattrset(root_window, COLOR_PAIR(1));
		wprintw(root_window, "% 9.2f %s ", sizeroundd(txd, units), unitround(txd, units));

		/* Print IP */
		separe(root_window);

		/* Highlight public address */
		if(strncmp(client->network[i].ip, "10.", 3) && strncmp(client->network[i].ip, "172.16.", 7) && strncmp(client->network[i].ip, "192.168.", 7))
			wattrset(root_window, A_BOLD | COLOR_PAIR(7));

		wprintw(root_window, " %-16s  ", client->network[i].ip);
		wattrset(root_window, COLOR_PAIR(1));

		separe(root_window);

		/* Print Speed */
		if(!client->network[i].speed) {
			wattrset(root_window, RATE_COLD);
			wprintw(root_window, " Unknown");

		} else wprintw(root_window, " %ld Mbps", client->network[i].speed);

		/* Notify that scroll is enabled */
		if(network_displayed + 1 == network_maxdisplay)
			waddstr(root_window, " (scroll v)");

		wprintw(root_window, "\n");
		wclrtoeol(root_window);

		j++;
		network_displayed++;
	}

	/* nothing displayed */
	if(!j) {
		if(client->lasttime < time(NULL) - 30)
			wattrset(root_window, A_BOLD | COLOR_PAIR(4));

		else if(client->lasttime < time(NULL) - 5)
			wattrset(root_window, A_BOLD | COLOR_PAIR(3));

		else wattrset(root_window, A_BOLD | COLOR_PAIR(7));

		wprintw(root_window, " %-14s ", client->hostname);
		separe(root_window);

		wattrset(root_window, RATE_COLD);
		wprintw(root_window, " Nothing to display\n");
	}
}

void print_whole_data(client_t *root, int units, int display) {
	unsigned int i;
	wmove(root_window, 0, 0);

	build_header(root_window);

	for(i = 0; i < root->count; i++)
		print_client_summary(&root->clients[i]);

	split(root_window);

	/* if only displaying summary, stopping here */
	if(display == DISPLAY_SUMMARY) {
		erase_anythingelse(root_window);
		wrefresh(root_window);
		return;
	}

	build_netheader(root_window);

	network_skpped = 0;
	network_displayed = 0;

	/* console height minus error minus headers minus summary */
	network_maxdisplay = (__maxy - 1) - 1 - 4 - root->count;

	for(i = 0; i < root->count; i++) {
		print_client_network(&root->clients[i], units);

		/* serparation line */
		if(network_displayed < network_maxdisplay) {
			split(root_window);
			network_displayed++;
		}
	}

	erase_anythingelse(root_window);

	wrefresh(root_window);
}
