#ifndef __RTSOCKET_H
	#define __RTSOCKET_H
		
	void convert_packed(netinfo_packed_t *packed);
	void convert_packed_net(rtinfo_network_legacy_t *net);
	void convert_header(netinfo_packed_t *packed);

	uint16_t packed_speed_rtinfo(netinfo_speed_t speed);
#endif
