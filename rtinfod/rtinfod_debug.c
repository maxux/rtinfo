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
#include <time.h>
#include <stdint.h>
#include <rtinfo.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <ctype.h>
#include "rtinfod.h"
#include "../rtinfo-common/socket.h"
#include "rtinfod_input.h"
#include "rtinfod_debug.h"
#include "rtinfod_network.h"

void dump(unsigned char *data, unsigned int len) {
	unsigned int i, j;
	
	printf("[+] Data dump [%p -> %p] (%u bytes)\n", data, data + len, len);
	printf("[ ] 0x0000: ");
	
	for(i = 0; i < len;) {
		printf("0x%02x ", data[i++]);
		
		if(i % 16 == 0) {
			printf("|");
			
			for(j = i - 16; j < i; j++)
				printf("%c", ((isalnum(data[j]) ? data[j] : '.')));
						
			printf("|\n[ ] 0x%04x: ", i);
		}
	}
	
	if(i % 16) {
		printf("%-*s", 5 * (16 - (i % 16)), " ");
		
		printf("|");
		
		for(j = i - (i % 16); j < len; j++)
			printf("%c", ((isalnum(data[j]) ? data[j] : '.')));
					
		printf("%-*s|\n", 16 - (len % 16), " ");
	}
}

void debug_input(unsigned char *buffer, unsigned int recvsize) {
	netinfo_packed_t *cast;
	netinfo_packed_net_t *net;
	rtinfo_network_legacy_t *read;
	uint32_t i;
	char strip[16], ifname[64];
	
	dump(buffer, recvsize);
		
	cast = (netinfo_packed_t*) buffer;
	printf("[+] Options  : %u\n", be32toh(cast->options));
	printf("[+] Hostname : %s\n", cast->hostname);
	printf("[+] ClientID : %u\n", be32toh(cast->clientid)); // FIXME
	printf("[+] Version  : %u\n\n", be32toh(cast->version));
	
	if(be32toh(cast->options) & QRY_SOCKET)
		return;
	
	if(be32toh(cast->options) & USE_NETWORK) {
		net = (netinfo_packed_net_t *) cast;
		
		printf("[+] Network packet data:\n");
		printf("[ ] Interfaces: %d\n", be32toh(net->nbiface));
		
		read = net->net;
		
		for(i = 0; i < be32toh(net->nbiface); i++) {
			if(!inet_ntop(AF_INET, &read->ip, strip, sizeof(strip)))
				fprintf(stderr, "[-] Cannot extract ip !\n");
			
			if(read->name_length > sizeof(ifname)) {
				fprintf(stderr, "[-] Interface name too long\n");
				goto next_read;
			}
			
			strncpy(ifname, read->name, read->name_length);
			ifname[read->name_length] = '\0';
			
			printf("[ ] Name : %s [%s]\n", read->name, strip);
			printf("[ ] Rate : %llu / %llu\n", be64toh(read->up_rate), be64toh(read->down_rate));
			printf("[ ] Data : %llu / %llu\n", be64toh(read->current.up), be64toh(read->current.down));
			printf("[ ] Speed: %u\n\n", packed_speed_rtinfo(read->speed));
			
			next_read:
			read = (rtinfo_network_legacy_t*) ((char*) read + sizeof(rtinfo_network_legacy_t) + read->name_length);
		}
		
	} else {
		printf("[+] Summary packet data:\n");
		
		printf("[ ] CPU Count  : %u\n", be32toh(cast->nbcpu));
		
		for(i = 0; i < be32toh(cast->nbcpu); i++)
			printf("[ ] CPU % 6d : %d\n", i, cast->cpu[i].usage);
		
		
		printf("[ ] Memory RAM : %llu / %llu\n", be64toh(cast->memory.ram_used), be64toh(cast->memory.ram_total));
		printf("[ ] Memory SWAP: %llu / %llu\n", be64toh(cast->memory.swap_free), be64toh(cast->memory.swap_total));
		printf("[ ] Load Avg.  : %.2f / %.2f / %.2f\n", ((float) be32toh(cast->loadavg[0]) / 100), ((float) be32toh(cast->loadavg[1]) / 100), ((float) be32toh(cast->loadavg[2]) / 100));
		printf("[ ] Battery    : %u / %u / %u / %llu\n", be32toh(cast->battery.charge_full), be32toh(cast->battery.charge_now), cast->battery.load, be64toh(cast->battery.status));
		
		printf("[ ] CPU Temp   : %hu / %hu\n", be16toh(cast->temp_cpu.cpu_average), be16toh(cast->temp_cpu.critical));
		printf("[ ] HDD Temp   : %hu / %hu\n", be16toh(cast->temp_hdd.hdd_average), be16toh(cast->temp_hdd.peak));
		
		printf("[ ] Uptime     : %u\n", be32toh(cast->uptime.uptime));
		printf("[ ] Timestamp  : %u\n\n", be32toh(cast->timestamp));
	}
}
