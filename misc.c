#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "misc.h"

void diep(char *str) {
	perror(str);
	exit(1);
}

long long sum_line(char *line) {
	long long sum = 0;
	short i = 0;
	
	/* Skipping head spaces */
	while(*(line + i) && isspace(*(line + i)))
		i++;
	
	/* Calculating */
	while(*(line + i)) {
		sum += atoll(line + (i++));
		
		while(*(line + i) && !isspace(*(line + i)))
			i++;
	}
	
	return sum;
}

long long indexll(char *line, int index) {
	short i = 0, j = 0;
	
	for(i = 0; i < index; i++) {
		/* Skipping non-spaces */
		while(*(line + j) && !isspace(*(line + j)))
			j++;
		
		/* Skipping contingnous spaces */
		while(*(line + j) && isspace(*(line + j)))
			j++;
	}
	
	return (*(line + j)) ? atoll(line + j) : 0;
}

char * file_get(char *filename, char *data, size_t size) {
	FILE *fp;
	
	fp = fopen(filename, "r");
	if(!fp) {
		perror(filename);
		return NULL;
	}
	
	if(!fgets(data, size, fp)) {
		fprintf(stderr, "Cannot read %s\n", filename);
		return NULL;
	}
	
	fclose(fp);
	
	return data;
}

char * getinterfacename(char *line) {
	int length = 0, i = 0, j;
	char *name;
	
	/* Skipping spaces */
	while(*(line + i) && isspace(*(line + i)))
		i++;
	
	/* Saving start name position */
	j = i;
	
	/* Reading name length */
	while(*(line + i) && *(line + i++) != ':')
		length++;
	
	/* Copy name */
	name = (char*) malloc((sizeof(char) * length) + 1);
	strncpy(name, line + j, length);
	*(name + length) = '\0';
	
	return name;
}



