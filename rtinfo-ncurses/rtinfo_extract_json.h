#ifndef __RTINFO_NCURSES_EXTRACTJSON_H
	#define __RTINFO_NCURSES_EXTRACTJSON_H
	
	client_t * extract_json(char *text);
	
	#define json_check_object(x)    if(!json_is_object(x)) { \
					                fprintf(stderr, "[-] json error: not an object\n"); \
					                return NULL; \
	                                }

	#define json_check_array(x)     if(!json_is_array(x)) { \
					                fprintf(stderr, "[-] json error: not an array\n"); \
					                return NULL; \
	                                }

	#define json_check_string(x)    if(!json_is_string(x)) { \
					                fprintf(stderr, "[-] json error: not a string\n"); \
					                return NULL; \
	                                }

	#define json_string_obj(x, y)      json_string_value(json_object_get(x, y))
	#define json_long_obj(x, y)        (long) json_number_value(json_object_get(x, y))
	#define json_ulong_obj(x, y)       (unsigned long) json_number_value(json_object_get(x, y))
	#define json_ulonglong_obj(x, y)   (unsigned long long) json_number_value(json_object_get(x, y))
	
#endif
