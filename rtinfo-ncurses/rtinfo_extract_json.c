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
#include <jansson.h>
#include "rtinfo_client_data.h"
#include "rtinfo_extract_json.h"
#include "rtinfo_display.h"

client_data_t * extract_json_head(json_t *node, client_data_t *client) {
	client->hostname = strdup(json_string_obj(node, "hostname"));
	client->remoteip = strdup(json_string_obj(node, "remoteip"));
	client->lasttime = json_ulong_obj(node, "lasttime");
	
	// printf("[+] Head: %s <> %s\n", client->hostname, client->remoteip);
	
	return client;
}

client_data_t * extract_json_summary(json_t *node, client_data_t *client) {
	json_t *subnode, *subnode2;
	size_t i;
	
	/* Single data */
	client->summary.uptime = json_ulong_obj(node, "uptime");
	client->summary.time   = json_ulong_obj(node, "time");
	
	/* Sub data */
	subnode = json_object_get(node, "memory");
	json_check_object(subnode);
	
	client->summary.ram_total  = json_ulonglong_obj(subnode, "ram_total");
	client->summary.ram_used   = json_ulonglong_obj(subnode, "ram_used");
	client->summary.swap_total = json_ulonglong_obj(subnode, "swap_total");
	client->summary.swap_free  = json_ulonglong_obj(subnode, "swap_free");
	
	/* printf("[+] Memory: %llu %llu %llu %llu\n", client->summary.ram_total, client->summary.ram_used,
	                                            client->summary.swap_total, client->summary.swap_free); */
	
	subnode = json_object_get(node, "battery");
	json_check_object(subnode);
	
	client->summary.battery_full   = json_ulong_obj(subnode, "charge_full");
	client->summary.battery_now    = json_ulong_obj(subnode, "charge_now");
	client->summary.battery_load   = json_ulong_obj(subnode, "load");
	client->summary.battery_status = json_ulong_obj(subnode, "status");
	
	/* printf("[+] Battery: %lu %lu %ld %lu\n", client->summary.battery_full, client->summary.battery_now,
	                                         client->summary.battery_load, client->summary.battery_status); */
	
	subnode = json_object_get(node, "cpu_usage");
	json_check_array(subnode);
	
	client->summary.cpu_count = json_array_size(subnode) - 1;
	client->summary.cpu_usage = (long) json_number_value(json_array_get(subnode, 0));
	
	// printf("[+] CPU: %ld%% (Count: %ld)\n", client->summary.cpu_usage, client->summary.cpu_count);
	
	subnode = json_object_get(node, "loadavg");
	json_check_array(subnode);
	
	for(i = 0; i < json_array_size(subnode) && i < 3; i++)
		client->summary.loadavg[i] = (float) json_number_value(json_array_get(subnode, i));
	
	/* printf("[+] Load: %.2f %.2f %.2f\n", client->summary.loadavg[0], client->summary.loadavg[1],
	                                     client->summary.loadavg[2]); */
	
	subnode = json_object_get(node, "sensors");
	json_check_object(subnode);
	
	subnode2 = json_object_get(subnode, "cpu");
	json_check_object(subnode2);
	
	client->summary.sensors_cpu_avg  = json_ulong_obj(subnode2, "average");
	client->summary.sensors_cpu_crit = json_ulong_obj(subnode2, "critical");
	
	subnode2 = json_object_get(subnode, "hdd");
	json_check_object(subnode2);
	
	client->summary.sensors_hdd_peak = json_ulong_obj(subnode2, "peak");
	client->summary.sensors_hdd_avg  = json_ulong_obj(subnode2, "average");
	
	/* printf("[+] Sensors: %lu %lu / %lu %lu\n", client->summary.sensors_cpu_avg, client->summary.sensors_cpu_crit,
	                                           client->summary.sensors_hdd_avg, client->summary.sensors_hdd_peak); */
	
	return client;
}

client_data_t * extract_json_network(json_t *node, client_data_t *client) {
	json_t *subnode, *interface;
	size_t i;
	
	subnode = json_object_get(node, "network");
	json_check_array(subnode);
	
	client_init_network(client, json_array_size(subnode));
	
	for(i = 0; i < json_array_size(subnode); i++) {
		interface = json_array_get(subnode, i);
		json_check_object(interface);
		
		client->network[i].ifname  = strdup(json_string_obj(interface, "name"));
		client->network[i].ip      = strdup(json_string_obj(interface, "ip"));
		client->network[i].rx_data = json_ulonglong_obj(interface, "rx_data");
		client->network[i].tx_data = json_ulonglong_obj(interface, "tx_data");
		client->network[i].rx_rate = json_ulonglong_obj(interface, "rx_rate");
		client->network[i].tx_rate = json_ulonglong_obj(interface, "tx_rate");
		client->network[i].speed   = json_ulonglong_obj(interface, "speed");
		
		/* printf("[+] Network [%s]: %llu, %llu (%llu/%llu) | %s, %lu\n", client->network[i].ifname, 
		                                client->network[i].rx_data, client->network[i].tx_data,
		                                client->network[i].rx_rate, client->network[i].tx_rate,
		                                client->network[i].ip, client->network[i].speed); */
	}
	
	return client;
}

client_t * extract_json(char *text) {
	json_t *root, *rtinfo, *node;
	json_error_t error;
	unsigned int i;
	client_t *clients;
	
	// printf("<%s>\n", text);
	
	if(!(root = json_loads(text, 0, &error))) {
		display_error("json errors\n");
		return NULL;
	}
	
	json_check_object(root);
	
	rtinfo = json_object_get(root, "rtinfo");
	json_check_array(rtinfo);
	
	clients = client_create(json_array_size(rtinfo));
	
	for(i = 0; i < json_array_size(rtinfo); i++) {
		node = json_array_get(rtinfo, i);
		json_check_object(node);
		
		extract_json_head(node, &clients->clients[i]);
		extract_json_summary(node, &clients->clients[i]);
		extract_json_network(node, &clients->clients[i]);
	}
	
	json_decref(root);
	
	return clients;
}
