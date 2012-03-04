#ifndef __RTINFO_H
	#define __RTINFO_H
	
	#define MEMORY_FILE	"/proc/meminfo"
	#define CPU_FILE	"/proc/stat"
	#define NET_FILE	"/proc/net/dev"

	#define INTERVAL	2000	/* Millisecondes */

	/* struct winsize ws; */

	typedef struct {
		/* CPU */
		unsigned char *cpu_usage;
		double *cpu_total_now;
		double *cpu_idle_now;
		double *cpu_total_old;
		double *cpu_idle_old;

		/* Network */
		char **ifname;
		double *if_download_now;
		double *if_upload_now;
		double *if_download_last;
		double *if_upload_last;

		size_t *download_rate;
		size_t *upload_rate;

		/* RAM */
		size_t memory_total;
		size_t memory_used;
		size_t swap_total;
		size_t swap_free;

	} sysrt_t;
#endif
