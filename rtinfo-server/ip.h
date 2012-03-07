#ifndef __IP_HEADER
	#define __IP_HEADER
	
	int ip_mkmask(int imask);
	int ip_mkip(char *ip);
	int ip_parsecidr(char *input, unsigned int *ip, unsigned int *mask);
#endif
