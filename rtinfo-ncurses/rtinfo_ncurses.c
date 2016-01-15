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
#include <unistd.h>
#include <getopt.h>
#include <ncurses.h>
#include <signal.h>
#include <rtinfo.h>
#include "../rtinfo-common/socket.h"
#include "rtinfo_ncurses.h"
#include "rtinfo_client_data.h"
#include "rtinfo_extract_json.h"
#include "rtinfo_display.h"
#include "rtinfo_socket.h"
#include "rtinfo_units.h"

#define OUTPUT_DEFAULT_HOST     "localhost"

static struct option long_options[] = {
	{"host", required_argument, 0, 'h'},
	{"port", required_argument, 0, 'p'},
	{"bits", no_argument,       0, 'b'},
	{0, 0, 0, 0}
};

extern int network_skip;

void dummy(int signal) {
	switch(signal) {
		case SIGINT:
			endwin();
			exit(EXIT_SUCCESS);
		break;
		
		case SIGWINCH:
			endwin();
			refresh();
		break;
	}
}

void print_usage(char *app) {
	(void) app;
	
	printf("rtinfo-ncurses (version %.3f)\n", VERSION);
	
	printf(" --host <host>   specify remote daemon host (default: %s)\n", OUTPUT_DEFAULT_HOST);
	printf(" --port <port>   specify remote daemon port (default: %d)\n", OUTPUT_DEFAULT_PORT);
	printf(" --bits          use bits for network units (default are bytes)\n");
	
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	char *json;
	int key;
	client_t *root;
	int i, option_index = 0;
	
	char *server = OUTPUT_DEFAULT_HOST;
	int port = OUTPUT_DEFAULT_PORT;
	int units = UNITS_BYTES;
	
	/* Checking arguments */
	while(1) {
		i = getopt_long(argc, argv, "h:pb", long_options, &option_index);

		/* Detect the end of the options. */
		if(i == -1)
			break;

		switch(i) {
			case 'h':
				server = strdup(optarg);
				break;
				
			case 'p':
				port = atoi(optarg);
				break;
				
			case 'b':
				units = UNITS_BITS;
				break;

			/* unrecognized option */
			case '?':
				print_usage(argv[0]);
				return 1;
			break;

			/* error */
			default:
				abort();
		}
	}
	
	/* Handling Resize Signal */
	signal(SIGINT, dummy);
	signal(SIGWINCH, dummy);
	
	/* Initializing ncurses */
	initconsole();
	
	while(1) {
		/* clear error message */
		display_clrerror();
		
		/* grabbing data */
		if((json = socket_rtinfo(server, port))) {
			if((root = extract_json(json))) {
				print_whole_data(root, units);
				client_delete(root);
			}
			
			free(json);
		}
		
		/* waiting next iteration with user input for scroll */
		/* timeout is set to 1 second */
		if((key = getch()) != ERR) {
			if(key == KEY_UP && network_skip > 0)
				network_skip--;
			
			else if(key == KEY_DOWN)
				network_skip++;
		}
	}

	endwin();
	
	return 0;
}
