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
#include "client.h"
#include "client_network.h"

void diep(char *str) {
	perror(str);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	int port;
	
	if((int) rtinfo_version() != 3 || rtinfo_version() < 3.44) {
		fprintf(stderr, "[-] Require librtinfo 3 (>= 3.44)\n");
		return 1;
	}
	
	if(argc < 2) {
		fprintf(stderr, "Usage: ./%s server [port]\n", argv[0]);
		exit(EXIT_FAILURE);
		
	} else port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;
	
	return networkside(argv[1], port);
}
