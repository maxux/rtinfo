/*
 * rtinfod is the daemon monitoring rtinfo remote clients
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
#include <stdint.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <rtinfo.h>
#include "../rtinfo-common/socket.h"
#include "rtinfod.h"
#include "rtinfod_input.h"
#include "rtinfod_output.h"
#include "rtinfod_stack.h"
#include "rtinfod_ip.h"
#include "rtinfod_network.h"

global_t global = {
	.clients = NULL,
	.debug   = 0,
	.verbose = 0,
};

static struct option long_options[] = {
	{"allow",         required_argument, 0, 'a'},
	{"client-listen", required_argument, 0, 'l'},
	{"remote-listen", required_argument, 0, 't'},
	{"client-port",   required_argument, 0, 'c'},
	{"remote-port",   required_argument, 0, 'r'},
	{"help",          no_argument,       0, 'h'},
	{"debug",         no_argument,       0, 'd'},
	{"verbose",       no_argument,       0, 'v'},
	{0, 0, 0, 0}
};

void diep(char *str) {
	perror(str);
	exit(EXIT_FAILURE);
}

void print_usage(char *app) {
	(void) app;
	
	printf("rtinfo Server (version %.2f)\n\n", SERVER_VERSION);
	
	printf(" --client-listen  specify client (rtinfo-client) listen address\n");
	printf(" --client-port    specify client (rtinfo-client) listen port\n");
	printf(" --remote-listen  specify remote (http) listen port\n");
	printf(" --remote-port    specify remote (http) listen port\n");
	printf(" --debug          debug messages (very verbose)\n");
	printf(" --verbose        be a little more verbose\n");
	printf(" --help           this message\n\n");
	
	printf("note: *-listen options can be specified multiple time\n");	
	// printf(" --allow          remote ip allowed (not working yet)\n");
	
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	thread_input_t thread_input_data = {
		.port = INPUT_DEFAULT_PORT,
	};
	thread_output_t thread_output_data = {
		.port = OUTPUT_DEFAULT_PORT,
	};
	
	// TODO
	// unsigned int *mask = NULL, *baseip = NULL;
	// int nballow = 0;
	
	int i;	
	int option_index = 0;
	
	if((int) rtinfo_version() != (int) REQUIRED_LIB_VERSION || rtinfo_version() < REQUIRED_LIB_VERSION) {
		fprintf(stderr, "[-] core: require librtinfo %d (>= %.2f)\n", (int) REQUIRED_LIB_VERSION, REQUIRED_LIB_VERSION);
		return 1;
	}
	
	/* Parsing options */
	while(1) {
		i = getopt_long(argc, argv, "a:c:r:hdv", long_options, &option_index);

		/* Detect the end of the options. */
		if(i == -1)
			break;

		switch(i) {
			/* New allowed client */
			case 'a':
				/* nballow++;
				
				mask   = (unsigned int*) realloc(mask, sizeof(unsigned int) * nballow);
				baseip = (unsigned int*) realloc(baseip, sizeof(unsigned int) * nballow);
				
				if(ip_parsecidr(optarg, (baseip + (nballow - 1)), (mask + (nballow - 1)))) {
					fprintf(stderr, "[-] cannot read allow entry\n");
					return 1;
				} */
				// TODO
			break;
			
			/* Specific port/debug */
			case 'c': thread_input_data.port  = atoi(optarg); break;
			case 'r': thread_output_data.port = atoi(optarg); break;
			case 'd': global.debug            = 1;            break;
			case 'v': global.verbose          = 1;            break;
			
			// client listen address
			case 'l':
				if(__bind_input_count < MAX_NETWORK_BIND)
					__bind_input[__bind_input_count++ - 1] = strdup(optarg);
			break;
			
			// remote listen address
			case 't':
				if(__bind_output_count < MAX_NETWORK_BIND)
					__bind_output[__bind_output_count++ - 1] = strdup(optarg);
			break;
			
			case 'h':
				print_usage(argv[0]);
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
	
	verbose("[+] core: initializing rtinfod version %f\n", SERVER_VERSION);
	debug("[+] core: starting threads\n");
		
	if(pthread_create(&thread_input_data.thread, NULL, init_input, (void *) &thread_input_data))
		diep("[-] core: input: pthread_create");
	
	if(pthread_create(&thread_output_data.thread, NULL, init_output, (void *) &thread_output_data))
		diep("[-] core: output: pthread_create");
	
	pthread_join(thread_input_data.thread, NULL);
	pthread_join(thread_output_data.thread, NULL);
	
	return 0;
}
