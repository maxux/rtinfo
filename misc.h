#ifndef __MISC_H
	#define __MISC_H
	
	long long sum_line(char *line);
	long long indexll(char *line, int index);
	char * file_get(char *filename, char *data, size_t size);
	char * getinterfacename(char *line);
	
	#define COLOR_RED	"\033[1;31m"
	#define COLOR_YELLOW	"\033[1;33m"
	#define COLOR_NONE	"\033[0m"
#endif
