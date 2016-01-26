/*
 * rtinfo is a small local/client "realtime" system health monitor
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
#include <rtinfo.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rtinfo_client.h"
#include "client_network.h"

static struct option long_options[] = {
	{"host",     required_argument, 0, 'h'},
	{"port",     required_argument, 0, 'p'},
	{"interval", required_argument, 0, 'i'},
	{"disk",     required_argument, 0, 'k'},
	{"daemon",   no_argument,       0, 'd'},
	{0, 0, 0, 0}
};

void diep(char *str) {
	perror(str);
	exit(EXIT_FAILURE);
}

void print_usage(char *app) {
	(void) app;
	
	printf("rtinfo-client (v%.3f) command line", CLIENT_VERSION);
	
	printf(" --host <host>      specify remote daemon host (default: %s)\n", DEFAULT_HOST);
	printf(" --port <port>      specify remote daemon port (default: %d)\n", DEFAULT_PORT);
	printf(" --interval <msec>  interval between two measure (default is 1000 (1s))\n");
	printf(" --disk <prefix>    filter disk prefix (eg: 'sd' will match sda, sdb, ...)\n");
	printf(" --daemon           run the client in background\n");
	
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int i, option_index = 0;
	
	char *server = "localhost";
	int port = DEFAULT_PORT;
	int interval = 1000;
	char *disk = NULL;
	
	int daemon = 0;
	pid_t process = 0;
	
	if((int) rtinfo_version() != 4 || rtinfo_version() < 4.00) {
		fprintf(stderr, "[-] Require librtinfo 4 (>= 4.00)\n");
		return 1;
	}
	
	/* Checking arguments */
	while(1) {
		i = getopt_long(argc, argv, "h:p:i:dk:", long_options, &option_index);

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
				
			case 'i':
				interval = atoi(optarg);
				break;
			
			case 'k':
				disk = strdup(optarg);
				break;
			
			case 'd':
				daemon = 1;
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
	
	if(daemon) {
		// cannot fork
		if((process = fork()) < 0)
			diep("[-] fork");
		
		// closing parent
		if(process)
			return 0;
		
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}
	
	return networkside(server, port, interval, disk);
}
