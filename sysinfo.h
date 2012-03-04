#ifndef __SYSINFO_H
	#define __SYSINFO_H
	
	/* User defines */
	#define BATTERY_NAME		"BAT1"
	#define UPDATE_INTERVAL		1000000
	
	/* System defines */
	#define MEMORY_FILE	"/proc/meminfo"
	#define LOADAVG_FILE	"/proc/loadavg"
	#define CPU_FILE	"/proc/stat"
	#define NET_FILE	"/proc/net/dev"
	#define BATTERY_PATH	"/sys/class/power_supply/" BATTERY_NAME
	
	#include <stdint.h>
	
	/* CPU Structures */
	typedef struct info_cpu_node_t {
		uint64_t time_total;			/* Total CPU Time */
		uint64_t time_idle;			/* Idle CPU Time */
		
	} info_cpu_node_t;
	
	/* Note: first node (index 0) is the sum of all the CPUs */
	/*       'nbcpu' will always be (real numbers of cpu) + 1 for the sum */
	typedef struct info_cpu_t {
		unsigned char usage;			/* CPU Usage (in percent) */
		struct info_cpu_node_t current	__attribute__ ((packed));	/* Instant time values */
		struct info_cpu_node_t previous	__attribute__ ((packed));	/* Previous time values */
		
	} info_cpu_t;
	
	/* Memory (RAM/SWAP) Structure */
	typedef struct info_memory_t {
		uint64_t ram_total;		/* RAM Total (in kB) */
		uint64_t ram_used;		/* RAM Used [aka -/+ buffers/cache] (in kB) */
		uint64_t swap_total;		/* SWAP Total (in kB) */
		uint64_t swap_free;		/* SWAP Free (in kB) */
		
	} info_memory_t;
	
	/* Load Average Structure */
	typedef struct info_loadagv_t {
		float min_1;		/* Load average for 1 min ago */
		float min_5;		/* Load average for last 5 min */
		float min_15;		/* Load average for last 15 min */
		
	} info_loadagv_t;
	
	/* Battery Structure */
	typedef enum info_battery_status_t {
		FULL,
		CHARGING,
		DISCHARGING
		
	} info_battery_status_t;
	
	typedef struct info_battery_t {
		uint32_t charge_full;	/* Battery full charge value (dependent unit) */
		uint32_t charge_now;	/* Battery current charge value (dependent unit) */
		char load;		/* Battery current load (in percent) */
		
		enum info_battery_status_t status;	/* Battery current status */
		
	} info_battery_t;
	
	/* Network Structures */
	typedef struct info_network_node_t {
		uint64_t up;		/* Bytes transmitted by interface */
		uint64_t down;		/* Bytes received by interface */
		
	} info_network_node_t;
	
	typedef struct info_network_t {
		char *name;		/* Interface name */
		struct info_network_node_t current;	/* Number of bytes transfered over the interface */
		struct info_network_node_t previous;	/* Copy of previous bytes transfered over the interface */
		int up_rate;		/* Upload rate (in ko/s) */
		int down_rate;		/* Download rate (in ko/s) */
		
	} info_network_t;

	/* Functions prototypes */
	/* Initialize CPU structure (required to use CPU) */
	info_cpu_t * initinfo_cpu(int *nbcpu);
	
	/* Update CPU structure */
	info_cpu_t * getinfo_cpu(info_cpu_t *cpu, int nbcpu);
	
	/* Calculate the CPU Usage in percent for each CPU */
	info_cpu_t * mkinfo_cpu_usage(info_cpu_t *cpu, int nbcpu);
	
	/* Write structure with current values */
	info_memory_t * getinfo_memory(info_memory_t *memory);
	info_loadagv_t * getinfo_loadavg(info_loadagv_t *load);
	info_battery_t * getinfo_battery(info_battery_t *battery);
	
	/* Initialize Network structure (required to use network) */
	info_network_t * mkinfo_network_usage(info_network_t *net, int nbiface, int timewait);
	info_network_t * getinfo_network(info_network_t *net, int nbiface);
	info_network_t * initinfo_network(int *nbiface);
	
	/* Return a (struct tm) pointer to the current local time */
	struct tm * getinfo_time();
#endif
