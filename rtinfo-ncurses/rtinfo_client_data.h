#ifndef __RTINFO_NCURSES_CLIENTDATA_H
	#define __RTINFO_NCURSES_CLIENTDATA_H
	
	typedef struct client_summary_t {
		unsigned long cpu_usage;
		unsigned long cpu_count;
		
		unsigned long long ram_total;
		unsigned long long ram_used;
		unsigned long long swap_total;
		unsigned long long swap_free;
		
		float loadavg[3];
		
		time_t time;
		time_t uptime;
		
		unsigned long battery_full;
		unsigned long battery_now;
		signed   long battery_load;
		unsigned long battery_status;
		
		unsigned long sensors_cpu_avg;
		unsigned long sensors_cpu_crit;
		unsigned long sensors_hdd_avg;
		unsigned long sensors_hdd_peak;
		
	} client_summary_t;
	
	typedef struct client_network_t {
		char *ifname;
		char *ip;
		
		unsigned long long rx_data;
		unsigned long long tx_data;
		unsigned long long rx_rate;
		unsigned long long tx_rate;
		unsigned long speed;
		
	} client_network_t;
	
	typedef struct client_data_t {
		char *hostname;
		char *remoteip;
		time_t lasttime;
		
		struct client_summary_t summary;
		
		size_t ifcount;
		struct client_network_t *network;
		
	} client_data_t;
	
	typedef struct client_t {
		size_t count;
		struct client_data_t *clients;
		
	} client_t;
	
	client_t * client_create(size_t count);
	client_data_t * client_init_network(client_data_t *client, size_t ifcount);
	void client_delete(client_t *client);
#endif
