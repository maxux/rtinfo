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
#include <endian.h>
#include <rtinfo.h>
#include "../rtinfo-common/socket.h"

uint16_t packed_speed_rtinfo(netinfo_speed_t speed) {
	switch (speed) {
		case NETINFO_NET_SPEED_10:
			return 10;
			
		case NETINFO_NET_SPEED_100:
			return 100;
		
		case NETINFO_NET_SPEED_1000:
			return 1000;
		break;
		
		case NETINFO_NET_SPEED_2500:
			return 2500;
		break;
		
		case NETINFO_NET_SPEED_10000:
			return 10000;
		
		case NETINFO_NET_SPEED_UNK:
			return 0;
	}
	
	return 0;
}

void convert_packed(netinfo_packed_t *packed) {
	int i;
	
	packed->nbcpu = be32toh(packed->nbcpu);
	
	packed->memory.ram_total  = be64toh(packed->memory.ram_total);
	packed->memory.ram_used   = be64toh(packed->memory.ram_used);
	packed->memory.swap_total = be64toh(packed->memory.swap_total);
	packed->memory.swap_free  = be64toh(packed->memory.swap_free);
	
	for(i = 0; i < 3; i++)
		packed->loadavg[i]  = be32toh(packed->loadavg[i]);
	
	packed->battery.charge_full = be32toh(packed->battery.charge_full);
	packed->battery.charge_now  = be32toh(packed->battery.charge_now);
	packed->battery.status      = be64toh(packed->battery.status);
	
	packed->uptime.uptime       = be32toh(packed->uptime.uptime);
	
	packed->temp_cpu.critical    = be16toh(packed->temp_cpu.critical);
	packed->temp_cpu.cpu_average = be16toh(packed->temp_cpu.cpu_average);
	
	packed->temp_hdd.peak        = be16toh(packed->temp_hdd.peak);
	packed->temp_hdd.hdd_average = be16toh(packed->temp_hdd.hdd_average);
	
	packed->timestamp = be32toh(packed->timestamp);	
}

void convert_packed_net(rtinfo_network_legacy_t *net) {
	net->current.up   = htobe64(net->current.up);
	net->current.down = htobe64(net->current.down);
	net->up_rate      = htobe64(net->up_rate);
	net->down_rate    = htobe64(net->down_rate);
}

void convert_header(netinfo_packed_t *packed) {
	packed->options  = be32toh(packed->options);
	packed->clientid = be32toh(packed->clientid);
	packed->version  = be32toh(packed->version);
}
