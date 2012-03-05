#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <stropts.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "misc.h"
#include "sysinfo.h"


/*
 *	CPU
 */
info_cpu_t * initinfo_cpu(int *nbcpu) {
	FILE *fp;
	char data[256];
	info_cpu_t *cpu;
	int i;
	
	fp = fopen(CPU_FILE, "r");

	if(!fp) {
		perror(CPU_FILE);
		exit(1);
	}

	/* Counting number of cpu availble */
	*nbcpu = 0;
	while(fgets(data, sizeof(data), fp) != NULL) {
		/* Checking cpu line */
		if(strncmp(data, "cpu", 3) != 0)
			break;
		
		*nbcpu += 1;
	}
	
	/* Allocating */
	cpu = (info_cpu_t*) malloc(sizeof(info_cpu_t) * *nbcpu);
	if(!cpu)
		return NULL;
	
	/* Initializing values */
	for(i = 0; i < *nbcpu; i++) {
		cpu[i].current.time_total = 0;
		cpu[i].current.time_idle  = 0;
		
		cpu[i].previous.time_total = 0;
		cpu[i].previous.time_idle  = 0;
	}
	
	return cpu;
}

/* For each CPU, save old values, write on the info_cpu_node_t current value read from CPU_FILE */
info_cpu_t * getinfo_cpu(info_cpu_t *cpu, int nbcpu) {
	FILE *fp;
	char data[256];
	short i = 0, j = 0;

	fp = fopen(CPU_FILE, "r");

	if(!fp) {
		perror(CPU_FILE);
		exit(1);
	}

	while(fgets(data, sizeof(data), fp) != NULL && j < nbcpu) {
		/* Checking cpu line */
		if(strncmp(data, "cpu", 3) != 0)
			break;
		
		/* Searching first number */
		while(!isspace(*(data + i)))
			i++;
		
		/* Saving previous data */
		cpu[j].previous = cpu[j].current;
		
		/* cpu0    53464243 2698605 20822211 794620668 16726825 3778099 4582652 0 0 0	*/
		/*         [.......................] [ idle ]  [............................]	*/
		/* [name]  [..................... total cpu time ...........................]	*/
		
		cpu[j].current.time_total = sum_line(data + i);
		cpu[j].current.time_idle  = indexll(data, 4);

		j++;
	}

	fclose(fp);
	
	return cpu;
}

info_cpu_t * mkinfo_cpu_usage(info_cpu_t *cpu, int nbcpu) {
	int i;
	
	/* CPU Usage: 100 * (delta cpu time - delta idle time) / delta cpu time */
	for(i = 0; i < nbcpu; i++) {
		if(cpu[i].current.time_total != cpu[i].previous.time_total)
			cpu[i].usage = 100 * ((cpu[i].current.time_total - cpu[i].previous.time_total) - (cpu[i].current.time_idle - cpu[i].previous.time_idle)) / (cpu[i].current.time_total - cpu[i].previous.time_total);
			
		else cpu[i].usage = 0;
	}
		
	return cpu;
}

/*
 *	MEMORY
 */
info_memory_t * getinfo_memory(info_memory_t *memory) {
	FILE *fp;
	char data[32], missing;
	unsigned int _memfree = 0, _buffers = 0, _cached = 0;

	fp = fopen(MEMORY_FILE, "r");

	if(!fp) {
		perror(MEMORY_FILE);
		exit(1);
	}

	/* Init Memory */
	memory->ram_used  = 0;	/* Init Used ram to zero */
	memory->swap_free = 0;	/* Init free Swap */
	missing = 6;		/* Numbers of lines to read */

	while(missing && fgets(data, sizeof(data), fp) != NULL) {
		if(strncmp(data, "MemTotal:", 9) == 0) {
			memory->ram_total = atoll(data+10);
			missing--;

		} else if(strncmp(data, "MemFree:", 8) == 0) {
			_memfree = atoll(data+9);
			missing--;

		} else if(strncmp(data, "Buffers:", 8) == 0) {
			_buffers = atoll(data+9);
			missing--;

		} else if(strncmp(data, "Cached:", 7) == 0) {
			_cached = atoll(data+8);
			missing--;

		} else if(strncmp(data, "SwapTotal:", 10) == 0) {
			memory->swap_total = atoll(data+11);
			missing--;

		} else if(strncmp(data, "SwapFree:", 9) == 0) {
			memory->swap_free = atoll(data+9);
			missing--;
		}
	}

	fclose(fp);
	
	/* Checking if all data required is present */
	if(missing)
		return NULL;

	/* Calculating */
	memory->ram_used = memory->ram_total - _memfree - _buffers - _cached;
	
	return memory;
}

/*
 *	LOAD AVERAGE
 */
info_loadagv_t * getinfo_loadavg(info_loadagv_t *load) {
	char data[32];

	/* Init Load */
	load->min_1  = -1;
	load->min_5  = -1;
	load->min_15 = -15;
	
	if(!file_get(LOADAVG_FILE, data, sizeof(data)))
		return NULL;
	
	sscanf(data, "%f %f %f", &load->load[0], &load->load[1], &load->load[2]);
	
	return load;
}

/*
 *	DATE/TIME
 */
struct tm * getinfo_time() {
	time_t t;
	
	time(&t);
	return localtime(&t);
	
}

/*
 *	BATTERY
 */
info_battery_t * getinfo_battery(info_battery_t *battery) {
	FILE *fp;
	char data[32];

	/* Checking for battery presence */
	fp = fopen(BATTERY_PATH "/present", "r");
	if(!fp) {
		perror(BATTERY_PATH);
		exit(1);
	}
	
	fclose(fp);

	/* Reading current charge */
	if(!file_get(BATTERY_PATH "/charge_now", data, sizeof(data)))
		return NULL;
	
	battery->charge_now = atol(data);
	
	/* Reading full_charge */
	if(!file_get(BATTERY_PATH "/charge_full", data, sizeof(data)))
		return NULL;
	
	battery->charge_full = atol(data);
	
	/* Reading status */
	if(!file_get(BATTERY_PATH "/status", data, sizeof(data)))
		return NULL;
	
	if(!strncmp(data, "Full", 4))
		battery->status = FULL;
	
	else if(!strncmp(data, "Charging", 4))
		battery->status = CHARGING;
	
	else if(!strncmp(data, "Discharging", 4))
		battery->status = DISCHARGING;
	
	/* Calculating usage */
	battery->load = ((float) battery->charge_now / battery->charge_full) * 100;
	
	return battery;
}

/*
 *	NETWORK
 */
info_network_t * initinfo_network(int *nbiface) {
	FILE *fp;
	char data[256];
	info_network_t *net;
	int i;
	
	fp = fopen(NET_FILE, "r");

	if(!fp) {
		perror(NET_FILE);
		exit(1);
	}

	/* Counting number of interfaces availble */
	*nbiface = 0;
	while(fgets(data, sizeof(data), fp) != NULL) {
		/* Skip header */
		if(!strncmp(data, "Inter-|", 7))
			continue;
		
		if(!strncmp(data, " face |", 7))
			continue;
		
		*nbiface += 1;
	}
	
	/* Allocating */
	net = (info_network_t*) malloc(sizeof(info_network_t) * *nbiface);
	if(!net)
		return NULL;
	
	rewind(fp);
	
	/* Initializing values */
	i = 0;
	while(fgets(data, sizeof(data), fp) != NULL) {
		/* Skip header */
		if(!strncmp(data, "Inter-|", 7))
			continue;
		
		if(!strncmp(data, " face |", 7))
			continue;
		
		net[i].name = getinterfacename(data);
		
		net[i].current.up   = 0;
		net[i].current.down = 0;
		
		net[i].previous.up   = 0;
		net[i].previous.down = 0;
		
		i++;
	}
	
	return net;
}

/* For each CPU, save old values, write on the info_cpu_node_t current value read from CPU_FILE */
info_network_t * getinfo_network(info_network_t *net, int nbiface) {
	FILE *fp;
	char data[256];
	short i = 0;

	fp = fopen(NET_FILE, "r");

	if(!fp) {
		perror(NET_FILE);
		exit(1);
	}

	while(fgets(data, sizeof(data), fp) != NULL && i < nbiface) {
		/* Skip header */
		if(!strncmp(data, "Inter-|", 7))
			continue;
		
		if(!strncmp(data, " face |", 7))
			continue;
		
		/* Saving previous data */
		net[i].previous = net[i].current;
		
		net[i].current.up   = indexll(data, 10);
		net[i].current.down = indexll(data, 2);

		i++;
	}

	fclose(fp);
	
	getinfo_ipv4(net, nbiface);
	
	return net;
}

info_network_t * getinfo_ipv4(info_network_t *net, int nbiface) {
	int sockfd;
	struct ifconf ifconf;
	struct ifreq ifr[50];
	int ifs;
	int i, j;
	char ip[INET_ADDRSTRLEN];
	struct sockaddr_in *s_in;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		diep("socket");

	ifconf.ifc_buf = (char *) ifr;
	ifconf.ifc_len = sizeof(ifr);

	if(ioctl(sockfd, SIOCGIFCONF, &ifconf) == -1)
		diep("ioctl");

	ifs = ifconf.ifc_len / sizeof(ifr[0]);
	
	/* Reset IP */
	for(j = 0; j < nbiface; j++)
		*(net[j].ip) = '\0';
				
	for(i = 0; i < ifs; i++) {
		s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

		if(!inet_ntop(AF_INET, &s_in->sin_addr, ip, sizeof(ip)))
			diep("inet_ntop");

		for(j = 0; j < nbiface; j++) {
			if(!strcmp(ifr[i].ifr_name, net[j].name)) {
				strcpy(net[j].ip, ip);
				break;
			}
		}
	}
	
	close(sockfd);

	return net;
}

info_network_t * mkinfo_network_usage(info_network_t *net, int nbiface, int timewait) {
	int i;
	
	/* Network Usage: (current load - previous load) / timewait (milli sec) */
	for(i = 0; i < nbiface; i++) {
		net[i].down_rate = ((net[i].current.down - net[i].previous.down) / (timewait / 1000));
		net[i].up_rate   = ((net[i].current.up - net[i].previous.up) / (timewait / 1000));
		
		if(net[i].down_rate < 0)
			net[i].down_rate = 0;
		
		if(net[i].up_rate < 0)
			net[i].up_rate = 0;
	}

	return net;
}
