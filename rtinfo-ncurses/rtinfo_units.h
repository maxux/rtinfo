#ifndef __RTINFO_NCURSES_UNITS_H
	#define __RTINFO_NCURSES_UNITS_H
	
	double sizeroundd(long long size, int type);
	int uptime_value(time_t uptime);
	
	char * unitround(long long size, int type);
	char * uptime_unit(time_t uptime);
	
	#define UNITS_BYTES   1
	#define UNITS_BITS    2
	
	extern char *units_bits[];
	extern char *units_bytes[];
	extern char *uptime_units[];
#endif
