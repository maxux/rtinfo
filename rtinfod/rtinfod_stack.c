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
#include <stdint.h>
#include <time.h>
#include <rtinfo.h>
#include "rtinfod.h"
#include "rtinfod_stack.h"
#include "../rtinfo-common/socket.h"

client_t * stack_client(client_t *new) {
	new->next = global.clients;
	global.clients = new;
	
	return new;
}

client_t * unstack_client(client_t *remove) {
	// TODO
	return remove;
	
	/* client_t *temp;
	
	temp = clients;
	while(temp && ( != temp->id))
		temp = temp->next;
	
	if(temp) {
		free(temp);
		
	} else return NULL;
	
	return clients; */
}

client_t * stack_search(char *name) {
	client_t *temp;
	
	temp = global.clients;
	while(temp && strcmp(temp->name, name))
		temp = temp->next;
		
	return temp;
}

client_t * stack_newclient(char *hostname, uint32_t remoteip) {
	client_t *client;
	
	if(!(client = (client_t*) malloc(sizeof(client_t))))
		return NULL;
	
	client->name     = strdup(hostname);
	client->remoteip = remoteip;
	client->lasttime = time(NULL);
	
	client->summary_length = 0;
	client->summary        = NULL;
	
	client->net_length     = 0;
	client->net            = NULL;
	
	/* Add client on the stack, become the last client */
	if(!stack_client(client)) {
		// debug("[-] Stacking client failed\n");
		exit(EXIT_FAILURE);
	}
	
	return client;
}
