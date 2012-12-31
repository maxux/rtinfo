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
#include "rtinfo_client_data.h"

client_t * client_create(size_t count) {
	client_t *client;
	size_t i;
	
	client = (client_t*) malloc(sizeof(client_t) * count);
	client->count   = count;
	client->clients = (client_data_t*) malloc(sizeof(client_data_t) * count);
	
	for(i = 0; i < count; i++) {
		client->clients[i].hostname = NULL;
		client->clients[i].remoteip = NULL;
		client->clients[i].network  = NULL;
	}
	
	return client;
}

client_data_t * client_init_network(client_data_t *client, size_t ifcount) {
	client->ifcount = ifcount;
	client->network = (client_network_t*) calloc(ifcount, sizeof(client_network_t));
	
	return client;
}

void client_delete(client_t *client) {
	unsigned int i, j;
	
	for(i = 0; i < client->count; i++) {
		free(client->clients[i].hostname);
		free(client->clients[i].remoteip);
		
		for(j = 0; j < client->clients[i].ifcount; j++) {
			free(client->clients[i].network[j].ifname);
			free(client->clients[i].network[j].ip);
		}
		
		free(client->clients[i].network);
	}
	
	free(client->clients);
	free(client);
}
