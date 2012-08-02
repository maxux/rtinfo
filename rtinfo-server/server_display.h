#ifndef __DISPLAY_H
	#define __DISPLAY_H
	
	extern pthread_mutex_t mutex_screen;
	
	void show_packet(netinfo_packed_t *packed, struct sockaddr_in *remote, client_t *client);
	void show_packet_network(netinfo_packed_net_t *net, client_t *client);
	
	void refresh_whole();
	
	void initdisplay();
	void netmoveunder(client_t *root, int offset);
	void joining(client_t *client);
	
	void build_header(WINDOW *win);
	void build_netheader(WINDOW *win);
#endif
