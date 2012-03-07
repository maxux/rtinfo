#ifndef __DISPLAY_H
	#define __DISPLAY_H
	
	void show_packet(netinfo_packed_t *packed, struct sockaddr_in *remote, client_t *client);
	void show_packet_network(netinfo_packed_net_t *net, struct sockaddr_in *remote, client_t *client);
	
	void refresh_whole();
	void show_header();
	void show_net_header();
#endif
