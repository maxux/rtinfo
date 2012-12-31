#ifndef __RTINFOD_IP_H
	#define __RTINFOD_IP_H
	
	int ip_mkmask(int imask);
	int ip_mkip(char *ip);
	int ip_parsecidr(char *input, unsigned int *ip, unsigned int *mask);
#endif
