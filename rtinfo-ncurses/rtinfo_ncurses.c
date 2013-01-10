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
#include <ncurses.h>
#include <signal.h>
#include <rtinfo.h>
#include "../rtinfo-common/socket.h"
#include "rtinfo_ncurses.h"
#include "rtinfo_client_data.h"
#include "rtinfo_extract_json.h"
#include "rtinfo_display.h"
#include "rtinfo_socket.h"

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
	printf("rtinfo-ncurses (version %.3f)\n", VERSION);
	printf("%s server [port]\n", app);
	
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	char *server, *json;
	int port;
	client_t *root;
	
	/* Checking arguments */
	if(argc < 2) {
		fprintf(stderr, "Usage: %s server [port]\n", argv[0]);
		exit(EXIT_FAILURE);
		
	} else port = (argc > 2) ? atoi(argv[2]) : INPUT_DEFAULT_PORT;
	
	server = argv[1];
	
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
				print_whole_data(root);
				client_delete(root);
			}
			
			free(json);
		}
		
		/* waiting next iteration */
		usleep(1000000);
	}

	endwin();
	
	return 0;
}
