/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "rtinfo_units.h"

char *units_bytes[] = {"kB", "MB", "GB", "TB"};
char *units_bits[] = {"kb", "Mb", "Gb", "Tb"};

char *uptime_units[] = {"m", "h", "d"};

double sizefix(double value, int type) {
	if(type == UNITS_BITS)	
		return value * 8;
	
	return value;
}

double sizeroundd(long long size, int type) {
	unsigned int i;
	double result = size;
	
	for(i = 0; i < (sizeof(units_bytes) / 2) - 1; i++) {
		if(result / 1024 < 1023)
			return sizefix(result / 1024, type);
			
		else result /= 1024;
	}
	
	return sizefix(result, type);
}

char * unitround(long long size, int type) {
	unsigned int i;
	double result = size / 1024;	/* First unit is k */
	
	for(i = 0; i < sizeof(units_bytes) / sizeof(units_bytes[0]); i++) {
		if(result < 1023)
			break;
			
		result /= 1024;
	}
	
	if(type == UNITS_BITS)
		return units_bits[i];
	
	// fallback
	return units_bytes[i];
}

int uptime_value(time_t uptime) {
	if(uptime < 3600)
		return uptime / 60;
	
	if(uptime < 86400)
		return uptime / 3600;
	
	return uptime / 86400;
}

char * uptime_unit(time_t uptime) {
	if(uptime < 3600)
		return uptime_units[0];
	
	if(uptime < 86400)
		return uptime_units[1];
	
	return uptime_units[2];
}
