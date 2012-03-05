#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "../librtinfo/sysinfo.h"
#include "../librtinfo/misc.h"
#include "rtinfo-local.h"

int localside() {
	info_memory_t memory;
	info_loadagv_t loadavg;
	info_cpu_t *cpu;
	/* info_battery_t battery; */
	info_network_t *net;
	
	int nbcpu, nbiface, i;
	struct tm * timeinfo;
	
	/* char *battery_picto = "=+-"; */
	
	/* Initializing variables */
	net = initinfo_network(&nbiface);
	cpu = initinfo_cpu(&nbcpu);
	
	/* Working */	
	while(1) {	
		/* Pre-reading data */
		getinfo_cpu(cpu, nbcpu);
		getinfo_network(net, nbiface);

		/* Sleeping */
		usleep(UPDATE_INTERVAL);
		
		/* Reading CPU */
		getinfo_cpu(cpu, nbcpu);
		mkinfo_cpu_usage(cpu, nbcpu);
		
		/* Reading Network */
		getinfo_network(net, nbiface);
		mkinfo_network_usage(net, nbiface, UPDATE_INTERVAL / 1000);
		
		/* Reading Memory */
		if(!getinfo_memory(&memory))
			return 1;
		
		/* Reading Load Average */
		if(!getinfo_loadavg(&loadavg))
			return 1;
		
		/* Reading Battery State */
		/* if(!getinfo_battery(&battery))
			return 1; */
		
		/* Reading Time Info */
		timeinfo = getinfo_time();
		
		
		/* Display Data */
		printf("\rCPU: ");
		
		for(i = 1; i < nbcpu; i++) {
			if(cpu[i].usage > 85)
				printf(COLOR_RED);
				
			else if(cpu[i].usage > 50)
				printf(COLOR_YELLOW);
				
			printf("%3d%% " COLOR_NONE, cpu[i].usage);
		}
		
		/* printf(" | NET: ");
		for(i = 0; i < nbiface; i++)
			printf("[%s] % 8d ko/s - % 8d ko/s ", net[i].name, net[i].up_rate, net[i].down_rate); */
		
		printf("| RAM: ");
		if(((float) memory.ram_used / memory.ram_total) * 100 > 80)
			printf(COLOR_RED);
			
		else if(((float) memory.ram_used / memory.ram_total) * 100 > 50)
			printf(COLOR_YELLOW);
			
		printf("%4llu Mo (%2.0f%%)" COLOR_NONE, (unsigned long long) memory.ram_used / 1024, ((float) memory.ram_used / memory.ram_total) * 100);
		
		printf(" | SWAP: %3llu Mo (%2.0f%%)", (unsigned long long) (memory.swap_total - memory.swap_free) / 1024, (memory.swap_total) ? ((float) (memory.swap_total - memory.swap_free) / memory.swap_total) * 100 : 0);
		printf(" | AVG: %.2f %.2f", loadavg.load[0], loadavg.load[1]);
		
		/* printf(" | " BATTERY_NAME ": %c%d%%", battery_picto[battery.status], battery.load); */
		printf(" | %02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		
		printf("\033[K ");
		fflush(stdout);
	}
	
	return 0;
}
