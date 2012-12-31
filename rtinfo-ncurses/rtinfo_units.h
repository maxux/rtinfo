#ifndef __RTINFO_NCURSES_UNITS_H
	#define __RTINFO_NCURSES_UNITS_H
	
	double sizeroundd(long long size);
	int uptime_value(time_t uptime);
	
	char * unitround(long long size);
	char * uptime_unit(time_t uptime);
	
	extern char *units[];
	extern char *uptime_units[];
#endif
