#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <endian.h>
#include "../rtinfo-common/socket.h"
#include "server_socket.h"

#include <ncurses.h>
#include "server.h"

void convert_packed(netinfo_packed_t *packed) {
	int i;
	
	packed->nbcpu = be32toh(packed->nbcpu);
	
	// FIXME
	// for(i = 0; i < packed->nbcpu; i++)
		
	
	packed->memory.ram_total  = be64toh(packed->memory.ram_total);
	packed->memory.ram_used   = be64toh(packed->memory.ram_used);
	packed->memory.swap_total = be64toh(packed->memory.swap_total);
	packed->memory.swap_free  = be64toh(packed->memory.swap_free);
	
	for(i = 0; i < 3; i++)
		packed->loadavg[i]  = be32toh(packed->loadavg[i]);
	
	packed->battery.charge_full = be64toh(packed->battery.charge_full);
	packed->battery.charge_now  = be64toh(packed->battery.charge_now);
	packed->battery.status      = be64toh(packed->battery.status);
	
	packed->uptime.uptime       = be32toh(packed->uptime.uptime);
	
	packed->temp_cpu.critical    = be16toh(packed->temp_cpu.critical);
	packed->temp_cpu.cpu_average = be16toh(packed->temp_cpu.cpu_average);
	
	packed->temp_hdd.peak        = be16toh(packed->temp_hdd.peak);
	packed->temp_hdd.hdd_average = be16toh(packed->temp_hdd.hdd_average);
	
	packed->timestamp = be32toh(packed->timestamp);	
}

void convert_packed_net(netinfo_packed_net_t *net) {
	unsigned int i;
	
	for(i = 0; i < net->nbiface; i++) {
		net->net[i].current.up   = htobe64(net->net[i].current.up);
		net->net[i].current.down = htobe64(net->net[i].current.down);
		net->net[i].up_rate      = htobe64(net->net[i].up_rate);
		net->net[i].down_rate    = htobe64(net->net[i].down_rate);
		net->net[i].speed        = htobe16(net->net[i].speed);
	}
}

void convert_header(netinfo_packed_t *packed) {
	packed->options  = be32toh(packed->options);
	packed->clientid = be32toh(packed->clientid);
	packed->version  = be32toh(packed->version);
}
