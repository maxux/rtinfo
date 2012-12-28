#ifndef __RTSOCKET_H
	#define __RTSOCKET_H
	
	uint8_t rtinfo_speed_packed(uint16_t speed);
	size_t netbuild_assemble(rtinfo_network_legacy_t *read, rtinfo_network_if_t *intf);

	int netinfo_socket(char *server, int port, struct sockaddr_in *remote);
	int netinfo_send_packed(int sockfd, netinfo_packed_t *packed, size_t size, const struct sockaddr_in *remote);
	int netinfo_send_packed_net(int sockfd, netinfo_packed_net_t *net, size_t size, const struct sockaddr_in *remote);
	
	void convert_header(netinfo_packed_t *packed);
#endif
