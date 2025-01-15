#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <rtinfo.h>
#include <ctype.h>
#include <errno.h>
#include "../rtinfo-common/socket.h"
#include "byte_conversions.h"
#include "client_socket.h"
#include "client_network.h"
#include "rtinfo_client.h"

void dump(unsigned char *data, unsigned int len) {
	unsigned int i, j;

	printf("[+] Data dump [%p -> %p] (%u bytes)\n", data, data + len, len);
	printf("[ ] 0x0000: ");

	for(i = 0; i < len;) {
		printf("0x%02x ", data[i++]);

		if(i % 16 == 0) {
			printf("|");

			for(j = i - 16; j < i; j++)
				printf("%c", ((isalnum(data[j]) ? data[j] : '.')));

			printf("|\n[ ] 0x%04x: ", i);
		}
	}

	if(i % 16) {
		printf("%-*s", 5 * (16 - (i % 16)), " ");

		printf("|");

		for(j = i - (i % 16); j < len; j++)
			printf("%c", ((isalnum(data[j]) ? data[j] : '.')));

		printf("%-*s|\n", 16 - (len % 16), " ");
	}
}

int networkside(char *server, int port, int interval, char **disks, unsigned int sdisks) {
	netinfo_packed_t *packed_cast;
	char *packedbuild;
	short packedbuild_size;
	rtinfo_loadagv_t legacy_loadavg;

	rtinfo_cpu_t *cpu;
	short cpu_size;

	rtinfo_network_t *truenet;
	short legacy_size;

	rtinfo_disk_t *truedisk;
	rtinfo_disk_t **truedisks;
	short disk_size;
	int disks_total = 0;

	netinfo_packed_disk_t *netdisk;
	rtinfo_disk_legacy_t *diskdev;
	rtinfo_temp_hdd_t hddtemp, nvmetemp;

	char *netbuild = NULL;
	short netbuild_size = 0;
	short netbuild_effective_size = 0;
	netinfo_packed_net_t *netbuild_cast = NULL;
	rtinfo_network_legacy_t *readnet;
	size_t netsize;

	struct sockaddr_in remote;
	int sockfd;
	unsigned int i, j, oldnbiface;

	printf("[+] Starting rtinfo network client (version %.3f)\n", CLIENT_VERSION);
	printf("[ ] librtinfo version      : %.2f\n", rtinfo_version());

	printf("[ ] ---------------------------------------------\n");
	printf("[ ] netinfo_packed_t       : %lu bytes\n", sizeof(netinfo_packed_t));
	printf("[ ] netinfo_packed_net_t   : %lu bytes\n", sizeof(netinfo_packed_net_t));
	printf("[ ] rtinfo_network_legacy_t: %lu bytes\n", sizeof(rtinfo_network_legacy_t));
	printf("[ ] rtinfo_disk_legacy_t   : %lu bytes\n", sizeof(rtinfo_disk_legacy_t));
	printf("[ ] ---------------------------------------------\n");

	/*
	 * Initializing Network
	 */
	truenet = rtinfo_init_network();
	oldnbiface = -1;

	/*
	 * Initializing Disks
	 */
	if(!(truedisks = malloc(sizeof(rtinfo_disk_t *) * sdisks)))
		diep("disks malloc");

	for(i = 0; i < sdisks; i++) {
		truedisks[i] = rtinfo_init_disk(disks[i]);
		printf("[+] % 2d disks via filter    : %s\n", truedisks[i]->nbdisk, disks[i]);

		disks_total += truedisks[i]->nbdisk;
	}

	disk_size = sizeof(netinfo_packed_disk_t);

	for(i = 0; i < sdisks; i++) {
		truedisk = truedisks[i];

		for(j = 0; j < truedisk->nbdisk; j++) {
			disk_size += sizeof(rtinfo_disk_legacy_t) + strlen(truedisk->dev[j].name);
			printf("[ ] adding disk found      : %s\n", truedisk->dev[j].name);
		}
	}

	printf("[ ] netinfo_disk_legacy_t  : %u bytes (%u disks)\n", disk_size, disks_total);
	netdisk = malloc(disk_size);

	printf("[ ] ---------------------------------------------\n");

	/*
	 * Initializing CPU
	 */
	cpu = rtinfo_init_cpu();

	/* Calculate size of packed_t required */
	cpu_size = sizeof(rtinfo_cpu_legacy_t) * cpu->nbcpu;

	/* Building 'netbuild' memory area with dynamic extra content (like interfaces) */
	packedbuild_size = sizeof(netinfo_packed_t) + cpu_size;
	packedbuild = (char*) calloc(1, packedbuild_size);

	printf("[ ] packedbuild_size       : %u bytes\n", packedbuild_size);

	/* Casting to avoid shit code */
	packed_cast = (netinfo_packed_t*) packedbuild;

	/* Saving nbinterface to sending packet */
	packed_cast->nbcpu = htobe32(cpu->nbcpu);

	/* Writing hostname on netinfo_packed_t */
	if(gethostname(packed_cast->hostname, sizeof(packed_cast->hostname)))
		diep("gethostname");

	/*
	 * Initializing Socket
	 */
	printf("[+] connecting             : %s:%d\n", server, port);

	if((sockfd = netinfo_socket(server, port, &remote)) == -1) {
		fprintf(stderr, "[-] Cannot resolve host: %s\n", server);
		exit(EXIT_FAILURE);
	}

	/* Authentificating */
	packed_cast->options = htobe32(QRY_SOCKET);
	packed_cast->version = htobe32(CLIENT_VERSION);

	/* Waiting response from server */
	i = 0;

	do {
		printf("[+] sending request seq id : %d\n", i++);
		netinfo_send_packed(sockfd, packed_cast, packedbuild_size, &remote);

		if(recv(sockfd, packed_cast, packedbuild_size, 0) == -1) {
			if(errno != EAGAIN)
				diep("recv");

		} else errno = 0;

	} while(errno == EAGAIN);

	convert_header(packed_cast);

	/* Checking ACK answer */
	if(!(packed_cast->options & ACK_SOCKET)) {
		fprintf(stderr, "[-] Wrong response from server\n");
		exit(EXIT_FAILURE);
	}

	/* Checking version */
	if((int)((float) packed_cast->version) != (int) CLIENT_VERSION) {
		fprintf(stderr, "[-] Version client/server (%f/%f) mismatch\n", CLIENT_VERSION, (float) packed_cast->version);
		exit(EXIT_FAILURE);
	}

	printf("[+] server version match   : %d.x\n", packed_cast->version);


	/*
	 * Building options
	 */
	packed_cast->options   = htobe32(USE_SUMMARY);
	packed_cast->version   = htobe32(CLIENT_VERSION);

	// disks
	netdisk->options       = htobe32(USE_DISK);
	netdisk->version       = htobe32(CLIENT_VERSION);
	netdisk->nbdisk        = htobe32(disks_total);

	if(gethostname(netdisk->hostname, sizeof(netdisk->hostname)))
		diep("gethostname");

	/* Pre-reading data */
	rtinfo_get_cpu(cpu);
	rtinfo_get_network(truenet);

	for(i = 0; i < sdisks; i++)
		rtinfo_get_disk(truedisks[i]);

	rtinfo_init_temp_hdd(&hddtemp);
	rtinfo_init_temp_hdd(&nvmetemp);

	/* Working */
	while(1) {
		/* Sleeping */
		usleep(interval * 1000);

		/* Reading CPU */
		rtinfo_get_cpu(cpu);
		rtinfo_mk_cpu_usage(cpu);

		for(i = 0; i < cpu->nbcpu; i++)
			packed_cast->cpu[i].usage = cpu->dev[i].usage;

		/* Reading Network */
		rtinfo_get_network(truenet);
		rtinfo_mk_network_usage(truenet, interval);

		for(i = 0; i < sdisks; i++) {
			truedisk = truedisks[i];

			rtinfo_get_disk(truedisk);
			rtinfo_mk_disk_usage(truedisk, interval);
		}

		if(!rtinfo_get_memory(&packed_cast->memory))
			return 1;

		if(!rtinfo_get_loadavg(&legacy_loadavg))
			return 1;

		for(i = 0; i < 3; i++)
			packed_cast->loadavg[i] = (uint32_t)(legacy_loadavg.load[i] * 100);

		if(!rtinfo_get_uptime(&packed_cast->uptime))
			return 1;

		if(!rtinfo_get_battery(&packed_cast->battery, NULL))
			return 1;

		if(!rtinfo_get_temp_cpu(&packed_cast->temp_cpu))
			return 1;

		if(!rtinfo_get_temp_hdd(&hddtemp))
			return 1;

		if(!rtinfo_get_temp_nvme(&nvmetemp))
			return 1;

		packed_cast->temp_hdd.peak = hddtemp.peak;
		packed_cast->temp_hdd.hdd_average = hddtemp.hdd_average;

		if(nvmetemp.peak > 0 && nvmetemp.hdd_average > 0) {
			packed_cast->temp_hdd.peak += nvmetemp.peak;
			packed_cast->temp_hdd.hdd_average += nvmetemp.hdd_average;

			packed_cast->temp_hdd.peak /= 2;
			packed_cast->temp_hdd.hdd_average /= 2;
		}

		/* Reading Time Info */
		time((time_t*) &packed_cast->timestamp);

		/* Sending info_t packed */
		netinfo_send_packed(sockfd, packed_cast, packedbuild_size, &remote);

		/*
		 * Building Network packet from librtinfo response
		 */

		/* Calculate size of legacy_t required */
		if(oldnbiface != truenet->nbiface) {
			free(netbuild);

			/* Allocating bytes for struct and interface name */
			legacy_size = (sizeof(rtinfo_network_legacy_t) + IFNAMSIZ + 1) * truenet->nbiface;

			/* Building 'netbuild' memory area with dynamic extra content

				<---    netinfo_packed_net_t      ---> <--- [info_network_legacy_t 1][info_network_legacy_t 2][...] --->
				<--- sizeof(netinfo_packed_net_t) ---> <---         sizeof(info_network_legacy_t) * nbifaces        --->
			*/
			netbuild_size = sizeof(netinfo_packed_net_t) + legacy_size;
			netbuild = (char*) calloc(1, netbuild_size);

			printf("[ ] netbuild_size          : %u bytes\n", netbuild_size);

			/* Casting for clean code */
			netbuild_cast = (netinfo_packed_net_t*) netbuild;

			/* Writing hostname to sending packed */
			if(gethostname(netbuild_cast->hostname, sizeof(netbuild_cast->hostname)))
				diep("gethostname");

			netbuild_cast->options = htobe32(USE_NETWORK);
			netbuild_cast->version = htobe32(CLIENT_VERSION);
			netbuild_cast->nbiface = htobe32(truenet->nbiface);

			/* Saving value */
			oldnbiface = truenet->nbiface;
		}

		/* Formating data */
		netbuild_effective_size = sizeof(netinfo_packed_net_t) + (sizeof(rtinfo_network_legacy_t) * truenet->nbiface);
		readnet = netbuild_cast->net;

		for(i = 0; i < truenet->nbiface; i++) {
			netsize = netbuild_assemble(readnet, (truenet->net + i));
			netbuild_effective_size += readnet->name_length;

			/* dump(readnet, netsize); */

			readnet = (rtinfo_network_legacy_t*) ((char*) readnet + netsize);
		}

		/* dump(netbuild_cast, netbuild_effective_size); */

		/* Sending info_net_t Packet */
		netinfo_send_packed_net(sockfd, netbuild_cast, netbuild_effective_size, &remote);

		/*
		 * Building disk packet
		 */
		diskdev = netdisk->disk;

		for(i = 0; i < sdisks; i++) {
			truedisk = truedisks[i];

			for(j = 0; j < truedisk->nbdisk; j++) {
				netsize = netdisk_assemble(diskdev, truedisk->dev + j);
				diskdev = (rtinfo_disk_legacy_t *) ((char *) diskdev + netsize);
			}
		}

		netinfo_send_packed_net(sockfd, (netinfo_packed_net_t *) netdisk, disk_size, &remote);
	}

	rtinfo_free_cpu(cpu);
	rtinfo_free_network(truenet);
	rtinfo_free_disk(truedisk);

	free(netbuild);
	free(packedbuild);

	return 0;
}
