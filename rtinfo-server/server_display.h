#ifndef __DISPLAY_H
	#define __DISPLAY_H
	
	extern pthread_mutex_t mutex_screen;
	
	void show_packet(struct sockaddr_in *remote, client_t *client);
	void show_packet_network(client_t *client, char redraw);
	
	void redraw_network(client_t *root);
	
	void initdisplay();
	// void netmoveunder(client_t *root, int offset);
	void preparing_client(client_t *client);
	
	void build_header(WINDOW *win);
	void build_netheader(WINDOW *win);
#endif
