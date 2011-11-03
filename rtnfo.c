#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "rtnfo.h"

char * SkipSpace(char *ptr, short nb) {
	short i;

	for(i = 0; i < nb; i++) {
		while(*ptr == ' ' && *ptr != '\0')
			ptr++;
		
		while(*ptr != ' ' && *ptr != '\0')
			ptr++;

		if(*ptr == '\0')
			return NULL;
	}

	return ptr;
}

int ReturnInt(char *line) {
	return atoi(line);
}

void WriteHeadName(char *data, char *output) {
	while(*data == ' ')
		data++;
	
	while(*data != ' ' && *data != ':') {
		*output = *data;

		output++;
		data++;
	}

	*output = '\0';
}

void Handle_Ifaces(sysrt_t *sys) {
	FILE *fp;
	char data[256], **iface, *temp;
	double *ifup, *ifdown;
	short i;

	fp = fopen(NET_FILE, "r");

	if(!fp) {
		printf("Cannot open %s\n", NET_FILE);
		exit(1);
	}

	/* Skipping Header */
	for(i = 0; i < 2; i++)
		if(fgets(data, sizeof(data), fp) == NULL)
			return;
	
	iface  = sys->ifname;
	ifup   = sys->if_upload_now;
	ifdown = sys->if_download_now;

	while(fgets(data, sizeof(data), fp) != NULL && *iface != NULL) {
		WriteHeadName(data, *iface);

		temp = SkipSpace(data, 1);
		*ifdown = atof(temp);

		temp = SkipSpace(data, 9);
		*ifup = atof(temp);

		iface++;
		ifup++;
		ifdown++;
	}

	fclose(fp);
}

void Handle_CPU(sysrt_t *sys) {
	FILE *fp;
	char data[256];
	double *cpu_total, *cpu_idle;
	short i;

	fp = fopen(CPU_FILE, "r");

	if(!fp) {
		printf("Cannot open %s\n", CPU_FILE);
		exit(1);
	}

	cpu_total = sys->cpu_total_now;
	cpu_idle  = sys->cpu_idle_now;

	while(fgets(data, sizeof(data), fp) != NULL && *cpu_total != -1) {
		if(strncmp(data, "cpu", 3) != 0)
			break;

		*cpu_total = 0;

		for(i = 1; i < 8; i++)
			*cpu_total += atoi(SkipSpace(data, i));

		*cpu_idle  = atoi(SkipSpace(data, 4));

		cpu_total++;
		cpu_idle++;
	}

	fclose(fp);
}

void Handle_Memory(sysrt_t *sys) {
	FILE *fp;
	char data[32], missing;
	unsigned int _MemFree = 0, _Buffers = 0, _Cached = 0;

	fp = fopen(MEMORY_FILE, "r");

	if(!fp) {
		printf("Cannot open %s\n", MEMORY_FILE);
		exit(1);
	}

	/* Init Memory */
	sys->memory_used = 0;	/* Init Used ram to zero	*/
	missing = 6;		/* Numbers of lines to read	*/

	while(fgets(data, sizeof(data), fp) != NULL && missing > 0) {
		if(strncmp(data, "MemTotal:", 9) == 0) {
			sys->memory_total = ReturnInt(data+10);
			missing--;

		} else if(strncmp(data, "MemFree:", 8) == 0) {
			_MemFree = ReturnInt(data+9);
			missing--;

		} else if(strncmp(data, "Buffers:", 8) == 0) {
			_Buffers = ReturnInt(data+9);
			missing--;

		} else if(strncmp(data, "Cached:", 7) == 0) {
			_Cached = ReturnInt(data+8);
			missing--;

		} else if(strncmp(data, "SwapTotal:", 10) == 0) {
			sys->swap_total = ReturnInt(data+11);
			missing--;

		} else if(strncmp(data, "SwapFree:", 9) == 0) {
			sys->swap_free = ReturnInt(data+9);
			missing--;
		}
	}

	fclose(fp);

	/* Building Stats */
	sys->memory_used = sys->memory_total - _MemFree - _Buffers - _Cached;
}

unsigned char CountLines(char filename[]) {
	FILE *fp;
	char temp[4096];
	unsigned char i = 0;

	fp = fopen(filename, "r");

	if(!fp)
		return 0;

	while(fgets(temp, sizeof(temp), fp) != NULL)
		i++;
	
	fclose(fp);

	return i;
}

void Sync_Ifaces(sysrt_t *sys, unsigned char phase) {
	char **iface;
	double *dnow, *unow, *dold, *uold;
	size_t *drate, *urate;

	iface = sys->ifname;

	dnow = sys->if_download_now;
	unow = sys->if_upload_now;
	dold = sys->if_download_last;
	uold = sys->if_upload_last;

	drate = sys->download_rate;
	urate = sys->upload_rate;

	while(*iface != NULL) {
		if(phase == 0) {		/* Copy data		*/
			*dold = *dnow;
			*uold = *unow;
		
		} else if(phase == 1) {		/* Calculate rate	*/
			*drate = (*dnow - *dold) * ((float) 100000 / (INTERVAL * 100));
			*urate = (*unow - *uold) * ((float) 100000 / (INTERVAL * 100));

			drate++;
			urate++;
		}

		dnow++;
		unow++;
		dold++;
		uold++;

		iface++;
	}
}

void Sync_CPU(sysrt_t *sys, unsigned char phase) {
	double *tcpu_now, *icpu_now, *tcpu_old, *icpu_old;
	unsigned char *cpu_usage;

	cpu_usage = sys->cpu_usage;

	tcpu_now = sys->cpu_total_now;
	icpu_now = sys->cpu_idle_now;
	tcpu_old = sys->cpu_total_old;
	icpu_old = sys->cpu_idle_old;

	if(phase == 1)
		cpu_usage = sys->cpu_usage;

	while(*tcpu_now != -1) {
		if(phase == 0) {		/* Copy data		*/
			*tcpu_old = *tcpu_now;
			*icpu_old = *icpu_now;
		
		} else if(phase == 1) {		/* Calculate rate	*/
			*cpu_usage = (1000 * ((*tcpu_now - *tcpu_old) - (*icpu_now - *icpu_old)) / (*tcpu_now - *tcpu_old)) / 10;
			cpu_usage++;

		}

		tcpu_now++;
		icpu_now++;
		tcpu_old++;
		icpu_old++;
	}
}

void ShowData(sysrt_t *sys) {
	char **iface;
	size_t *drate, *urate;
	unsigned char *cpu, i, maxifa = 0;

	/* Get Terminal Size */
	/* ioctl(0, TIOCGWINSZ, &ws); */

	/* Clear Screen */
	printf("\033[H");

	/* Init Pointers */
	iface = sys->ifname;
	drate = sys->download_rate;
	urate = sys->upload_rate;
	cpu   = sys->cpu_usage;

	/* CPU */
	/* Display CPU Header */
	i = 0;
	while(*(cpu+i) != 255)
		printf("[cpu%-2d] ", i++);
	

	printf("\033[K\n");
	i = 0;

	/* Display CPU Values */
	while(*(cpu+i) != 255) {
		if(*(cpu+i) > 85)
			printf("\033[1;31m");

		else if(*(cpu+i) > 50)
			printf("\033[1;33m");

		printf("%4d%%   \033[0m", *(cpu+i));
		i++;
	}

	printf("\033[K\n\033[K\n");


	/* MEMORY */
	printf("[sys/ram] [sys/ram] [sys/swap] [sys/swap]\033[K\n");

	/* RAM Percent */
	if(((float) sys->memory_used / sys->memory_total) > 0.8)
		printf("\033[1;31m");
	
	else if(((float) sys->memory_used / sys->memory_total) > 0.5)
		printf("\033[1;33m");

	printf(" %-4.1f%% %3c %-5u %-2c \033[0m", ((float) sys->memory_used / sys->memory_total) * 100, ' ', (unsigned int) sys->memory_used / 1024, 'M');

	/* SWAP Percent */
	if(((float) (sys->swap_total - sys->swap_free) / sys->swap_total) > 0.8)
		printf("\033[1;31m");
		
	else if(((float) (sys->swap_total - sys->swap_free) / sys->swap_total) > 0.5)
		printf("\033[1;33m");


	printf(" %-4.1f%% %4c %-6u %-3c \033[0m\033[K\n", ((float) (sys->swap_total - sys->swap_free) / sys->swap_total) * 100, ' ', (unsigned int) (sys->swap_total - sys->swap_free) / 1024, 'M');


	/* NETWORK */
	printf("\033[K\n");

	i = 0;
	while(*(iface+i) != NULL) {
		if(strlen(*(iface+i)) > maxifa)
			maxifa = strlen(*(iface+i));

		i++;
	}

	while(*iface != NULL) {
		/* Name */
		if(*drate > 3072 || *urate > 3072)
			printf("\033[1;36m");

		printf("[net/%-*s]  \033[0m", maxifa, *iface);
		
		/* Download */
		if(*drate > 20971520)
			printf("\033[1;31m");

		else if(*drate > 1572864)
			printf("\033[1;33m");
		
		printf("%10.1f ko/s   \033[0m", (float) *drate / 1024);
		
		/* Upload */
		if(*urate > 20971520)
			printf("\033[1;31m");

		else if(*urate > 1572864)
			printf("\033[1;33m");

		printf("%10.1f ko/s\033[0m\033[K\n", (float) *urate / 1024);

		iface++;
		drate++;
		urate++;
		/* count++; */
	}

	

	printf("\033[K\n");
	/* count += 2;

	for(;count < ws.ws_row - 2; count++) 
		printf("\033[K\n"); */

}

int main(void) {
	sysrt_t system;
	unsigned char nb, i;

	/* Clear Screen */
	printf("\033[H\033[2J");

	/* Initializing Struct */
	/* Network */
	nb = CountLines(NET_FILE) - 2;						/* 2 is for header	*/

	system.ifname = (char **) malloc(sizeof(char *) * (nb + 1));		/* Interfaces names	*/
	for(i = 0; i < nb; i++)
		system.ifname[i] = (char *) malloc(sizeof(char) * 16);		/* 16 bytes max		*/
	
	system.ifname[nb]       = NULL;

	system.if_download_now  = (double *) malloc(sizeof(double) * nb);	/* Allocating status	*/
	system.if_upload_now    = (double *) malloc(sizeof(double) * nb);

	system.if_download_last = (double *) malloc(sizeof(double) * nb);
	system.if_upload_last   = (double *) malloc(sizeof(double) * nb);

	system.download_rate    = (size_t *) malloc(sizeof(size_t) * nb);
	system.upload_rate      = (size_t *) malloc(sizeof(size_t) * nb);

	/* CPU */
	nb = CountLines(CPU_FILE) - 7;							/* 7 is for useless data */
	
	system.cpu_total_now    = (double *) malloc(sizeof(double) * (nb + 1));		/* 1 for NULL */
	system.cpu_idle_now     = (double *) malloc(sizeof(double) * (nb + 1));

	system.cpu_total_old    = (double *) malloc(sizeof(double) * (nb + 1));
	system.cpu_idle_old     = (double *) malloc(sizeof(double) * (nb + 1));

	system.cpu_usage	= (unsigned char *) malloc(sizeof(unsigned char) * (nb + 1));

	system.cpu_total_now[nb] = -1;
	system.cpu_usage[nb]     = 255;

	/* Inittializing Counters */
	Handle_Ifaces(&system);
	Handle_CPU(&system);

	/* Working */
	while(1) {
		Sync_Ifaces(&system, 0);
		Sync_CPU(&system, 0);

		Handle_Ifaces(&system);
		Handle_Memory(&system);
		Handle_CPU(&system);

		Sync_Ifaces(&system, 1);
		Sync_CPU(&system, 1);

		ShowData(&system);

		usleep(INTERVAL * 1000);
	}

	return 0;
}
