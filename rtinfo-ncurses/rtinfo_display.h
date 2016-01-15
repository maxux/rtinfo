#ifndef __RTINFO_NCURSES_DISPLAY_H
	#define __RTINFO_NCURSES_DISPLAY_H
	
	#define	BATTERY_CHARGING            1
	
	void initconsole();
	void print_whole_data(client_t *root, int units);
	
	void display_perror(char *str);
	void display_error(char *str);
	void display_clrerror();
#endif
