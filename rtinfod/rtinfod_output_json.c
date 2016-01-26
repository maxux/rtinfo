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
#include <pthread.h>
#include <arpa/inet.h>
#include <jansson.h>
#include <rtinfo.h>
#include "../rtinfo-common/socket.h"
#include "rtinfod.h"
#include "rtinfod_network.h"
#include "rtinfod_output_json.h"

char *__avgvalue[] = {"1", "5", "15"};

static json_t *json_node_network(rtinfo_network_legacy_t *net, uint32_t nbiface) {
	rtinfo_network_legacy_t *read = net;
	char strip[16], ifname[64];
	json_t *array, *interface;
	uint32_t i;
	
	array = json_array();
	
	for(i = 0; i < nbiface; i++) {
		interface = json_object();
		
		/* grab name */
		strncpy(ifname, read->name, read->name_length);
		ifname[read->name_length] = '\0';
		
		/* grab ip */
		inet_ntop(AF_INET, &read->ip, strip, sizeof(strip));
		
		/* filling json */
		json_object_set_new(interface, "name", json_string(ifname));
		json_object_set_new(interface, "ip", json_string(strip));
		json_object_set_new(interface, "rx_data", json_integer(read->current.down));
		json_object_set_new(interface, "tx_data", json_integer(read->current.up));
		json_object_set_new(interface, "rx_rate", json_integer(read->down_rate));
		json_object_set_new(interface, "tx_rate", json_integer(read->up_rate));
		json_object_set_new(interface, "speed", json_integer(packed_speed_rtinfo(read->speed)));
		
		json_array_append_new(array, interface);
		
		read = (rtinfo_network_legacy_t*) ((char*) read + sizeof(rtinfo_network_legacy_t) + read->name_length);
	}
	
	return array;
}

static json_t *json_node_disk(rtinfo_disk_legacy_t *disk, uint32_t nbdisk) {
	rtinfo_disk_legacy_t *dev = disk;
	char devname[64];
	json_t *array, *device;
	uint32_t i;
	
	array = json_array();
	
	for(i = 0; i < nbdisk; i++) {
		device = json_object();
		
		/* grab name */
		strncpy(devname, dev->name, dev->name_length);
		devname[dev->name_length] = '\0';
		
		/* filling json */
		json_object_set_new(device, "name", json_string(devname));
		json_object_set_new(device, "bytes_read", json_integer(dev->bytes_read));
		json_object_set_new(device, "bytes_written", json_integer(dev->bytes_written));
		json_object_set_new(device, "read_speed", json_integer(dev->read_speed));
		json_object_set_new(device, "write_speed", json_integer(dev->write_speed));
		json_object_set_new(device, "iops", json_integer(dev->iops));
		
		json_array_append_new(array, device);
		
		dev = (rtinfo_disk_legacy_t *) ((char*) dev + sizeof(rtinfo_disk_legacy_t) + dev->name_length);
	}
	
	return array;
}

static json_t *json_node_loadavg(uint32_t *loadavg) {
	unsigned int i;
	json_t *array;
	
	array = json_array();
	
	for(i = 0; i < 3; i++)
		json_array_append_new(array, json_real(loadavg[i] / (float) 100));
	
	return array;
}

static json_t *json_node_cpu(rtinfo_cpu_legacy_t *cpu, uint32_t nbcpu) {
	uint32_t i;
	json_t *array;
	
	array = json_array();
	
	for(i = 0; i < nbcpu; i++)
		json_array_append_new(array, json_integer(cpu[i].usage));
	
	return array;
}

static json_t *json_node_temp_cpu(rtinfo_temp_cpu_t *temp) {
	json_t *node;
	
	node = json_object();
	json_object_set_new(node, "average", json_integer(temp->cpu_average));
	json_object_set_new(node, "critical", json_integer(temp->critical));
	
	return node;
}

static json_t *json_node_temp_hdd(rtinfo_temp_hdd_t *temp) {
	json_t *node;
	
	node = json_object();
	json_object_set_new(node, "average", json_integer(temp->hdd_average));
	json_object_set_new(node, "peak", json_integer(temp->peak));
	
	return node;
}

static json_t *json_node_memory(rtinfo_memory_t *memory) {
	json_t *node;
	
	node = json_object();
	json_object_set_new(node, "ram_total", json_integer(memory->ram_total));
	json_object_set_new(node, "ram_used", json_integer(memory->ram_used));
	json_object_set_new(node, "swap_total", json_integer(memory->swap_total));
	json_object_set_new(node, "swap_free", json_integer(memory->swap_free));
	
	return node;
}

static json_t *json_node_battery(rtinfo_battery_t *battery) {
	json_t *node;
	
	node = json_object();
	json_object_set_new(node, "charge_full", json_integer(battery->charge_full));
	json_object_set_new(node, "charge_now", json_integer(battery->charge_now));
	json_object_set_new(node, "load", json_integer(battery->load));
	json_object_set_new(node, "status", json_integer(battery->status));
	
	return node;
}

/* Main JSON convertion */
char *output_json() {
	client_t *client;
	json_t *root, *rtinfo, *this;
	char strip[16], *output;
		
	rtinfo = json_array();
	
	/* Locking globals */
	pthread_mutex_lock(&global.mutex_clients);
	client = global.clients;
	
	// note: root is used as temporary variable on the loop
	while(client) {
		if(!client->summary || !client->net) {
			client = client->next;
			continue;
		}
		
		this = json_object();
		
		/* Summary part */
		json_object_set_new(this, "hostname", json_string(client->summary->hostname));
		json_object_set_new(this, "lasttime", json_integer(client->lasttime));
		
		inet_ntop(AF_INET, &client->remoteip, strip, sizeof(strip));
		json_object_set_new(this, "remoteip", json_string(strip));
		
		json_object_set_new(this, "memory", json_node_memory(&client->summary->memory));
		json_object_set_new(this, "cpu_usage", json_node_cpu(client->summary->cpu, client->summary->nbcpu));
		json_object_set_new(this, "loadavg", json_node_loadavg(client->summary->loadavg));
		json_object_set_new(this, "battery", json_node_battery(&client->summary->battery));
		
		root = this;
		this = json_object();
		
		json_object_set_new(this, "cpu", json_node_temp_cpu(&client->summary->temp_cpu));
		json_object_set_new(this, "hdd", json_node_temp_hdd(&client->summary->temp_hdd));
		
		json_object_set_new(root, "sensors", this);
		this = root;
		
		json_object_set_new(this, "uptime", json_integer(client->summary->uptime.uptime));
		json_object_set_new(this, "time", json_integer(client->summary->timestamp));
		
		/* Network part */
		json_object_set_new(root, "network", json_node_network(client->net->net, client->net->nbiface));
		
		/* Disk part */
		json_object_set_new(root, "disks", json_node_disk(client->disk->disk, client->disk->nbdisk));
		
		/* Appending */
		json_array_append_new(rtinfo, this);
		client = client->next;
	}
	
	/* Unlocking globals */
	pthread_mutex_unlock(&global.mutex_clients);
	
	root = json_object();
	json_object_set_new(root, "rtinfo", rtinfo);
	json_object_set_new(root, "version", json_real(SERVER_VERSION));
	json_object_set_new(root, "servertime", json_integer(time(NULL)));
	
	/* Saving json data */
	output = json_dumps(root, JSON_INDENT(2));
	
	/* Clearing */
	json_decref(root);
	
	return output;
}
