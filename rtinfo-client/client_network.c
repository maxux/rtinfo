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
#include <netdb.h>
#include <msgpack.h>
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

int netinfo_socket(char *server, int port, struct sockaddr_in *remote) {
	int sockfd;
	struct hostent *hent;
	struct timeval tv = {tv.tv_sec = 2, tv.tv_usec = 0};

	if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) remote, 0, sizeof(struct sockaddr_in));

	remote->sin_family = AF_INET;
	remote->sin_port   = htons(port);

	if((hent = gethostbyname(server)) == NULL)
		return -1;

	memcpy(&remote->sin_addr, hent->h_addr_list[0], hent->h_length);

	/* recv timeout */
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv)))
		diep("[-] setsockopt: SO_RCVTIMEO");

	return sockfd;
}

int netinfo_send_packed(int sockfd, const char *packed, size_t size, const struct sockaddr_in *remote) {
	if(sendto(sockfd, packed, size, 0, (const struct sockaddr *) remote, sizeof(struct sockaddr_in)) == -1) {
		perror("sendto");
	}

	return 0;
}

#define MB(x)   (x / 1024 / 1024)
#define MBf(x)  (x / 1024.0 / 1024.0)

typedef struct rtinfo_client_t {
	int sockfd;
	struct sockaddr_in remote;

} rtinfo_client_t;

int server_validate(char *server, int port, rtinfo_client_t *client) {
	// initializing socket
	printf("[+] connecting             : %s:%d\n", server, port);

	if((client->sockfd = netinfo_socket(server, port, &client->remote)) == -1) {
		fprintf(stderr, "[-] cannot resolve host: %s\n", server);
		exit(EXIT_FAILURE);
	}

	msgpack_sbuffer sbuf;
	msgpack_sbuffer_init(&sbuf);

	msgpack_packer pk;
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

	msgpack_pack_array(&pk, 3);

	// 1. Message type (10 = INIT, 11 = OK, 12 = REJECT, 20 = METRICS)
	msgpack_pack_uint8(&pk, 10);

	char xname[32];
	gethostname(xname, sizeof(xname));

	// 2. Hostname
	msgpack_pack_str(&pk, strlen(xname));
	msgpack_pack_str_body(&pk, xname, strlen(xname));

	// 3. Client Info/Version
	msgpack_pack_array(&pk, 2);

	// 3.1 Packet ID
	msgpack_pack_uint32(&pk, 1);

	// 3.2 librtinfo version
	msgpack_pack_float(&pk, rtinfo_version());

	printf("\n\n----------------------\n\n");
	printf("Serialized: %lu bytes\n\n", sbuf.size);

	ssize_t recvsize = 0;
	size_t retry = 0;
	char buffer[512];

	do {
		printf("[+] sending request seq id : %lu\n", retry++);
		netinfo_send_packed(client->sockfd, sbuf.data, sbuf.size, &client->remote);

		if((recvsize = recv(client->sockfd, buffer, sizeof(buffer), 0)) == -1) {
			if(errno != EAGAIN)
				diep("recv");

		} else errno = 0;

	} while(errno == EAGAIN);

	printf("\n\n----------------------\n\n");
	msgpack_sbuffer_destroy(&sbuf);

	// fallback to failure
	int confirm = 1;

	msgpack_unpacker *unp = msgpack_unpacker_new(1024);
	if(msgpack_unpacker_buffer_capacity(unp) < (size_t) recvsize) {
        if(!(msgpack_unpacker_reserve_buffer(unp, recvsize))) {
			diep("msgpack: reserve buffer");
		}
    }

	memcpy(msgpack_unpacker_buffer(unp), buffer, recvsize);
	msgpack_unpacker_buffer_consumed(unp, recvsize);

	msgpack_unpacked und;
	msgpack_unpack_return ret;
	msgpack_unpacked_init(&und);
	ret = msgpack_unpacker_next(unp, &und);

	switch(ret) {
		case MSGPACK_UNPACK_SUCCESS:
		{
			msgpack_object obj = und.data;
			printf("[+] validate: message unpacked (type: %d)\n", obj.type);

			printf("[.] -----------------------------\n");
			msgpack_object_print(stdout, und.data);
			printf("\n[.] -----------------------------\n");

			if(obj.type != MSGPACK_OBJECT_ARRAY) {
				printf("[-] validate: unpacked message is not an array\n");
				confirm = 4;
				break;
			}

			if(obj.via.array.size < 3) {
				printf("[-] validate: unpacked array is too small (size: %u)\n", obj.via.array.size);
				confirm = 5;
				break;
			}

			if(obj.via.array.ptr[0].type != MSGPACK_OBJECT_POSITIVE_INTEGER) {
				printf("[-] validate: invalid packet identifier format (type: %d)\n", obj.via.array.ptr[0].type);
				confirm = 6;
				break;
			}

			// ACCEPTED
			if(obj.via.array.ptr[0].via.u64 == 11) {
				confirm = 0;
			}

			// REJECTED
			if(obj.via.array.ptr[0].via.u64 == 12) {
				confirm = 1;
			}

			msgpack_object_array *ver = &obj.via.array.ptr[2].via.array;
			float libclient = ver->ptr[1].via.f64;

			char srvname[32];
			memset(srvname, 0, sizeof(srvname));
			strncpy(srvname, obj.via.array.ptr[1].via.str.ptr, obj.via.array.ptr[1].via.str.size);

			printf("[+] server validation      : %s\n", (confirm == 0) ? "accepted" : "rejected");
			printf("[+] server hostname        : %s\n", srvname);
			printf("[+] server librtinfo ver.  : %.2f\n", libclient);
		}
		break;

		case MSGPACK_UNPACK_CONTINUE:
			printf("CONTINUE\n");
			/* cheking capacity, reserve buffer, copy additional data to the buffer, */
			/* notify consumed buffer size, then call msgpack_unpacker_next(&unp, &und) again */
			break;

		case MSGPACK_UNPACK_PARSE_ERROR:
			printf("PARSE ERROR\n");
			break;

		case MSGPACK_UNPACK_EXTRA_BYTES:
			printf("EXTRA BYTES\n");
			break;

		case MSGPACK_UNPACK_NOMEM_ERROR:
			printf("NO MEM\n");
			break;
	}

	msgpack_unpacked_destroy(&und);
	msgpack_unpacker_free(unp);

	return confirm;
}

int client_process(rtinfo_client_t *client, int interval, char **xdisks, size_t sdisks) {
	rtinfo_cpu_t *cpu;
	rtinfo_disk_t *disks;
	rtinfo_rapl_t *pwr;
	rtinfo_network_t *net;
	rtinfo_uptime_t uptime;
	rtinfo_battery_t battery;
	rtinfo_memory_t memory;
	rtinfo_loadagv_t loadavg;;
	struct tm * timeinfo;
	uint32_t reqid = 1;

	printf("[+] initializing librtinfo-test (v%.2f)...\n", rtinfo_version());
	cpu = rtinfo_init_cpu();
	net = rtinfo_init_network();

	disks = rtinfo_init_disk("INIT", NULL);

	for(size_t diskid = 0; diskid < sdisks; diskid++) {
		disks = rtinfo_init_disk(xdisks[diskid], disks);
	}

	printf("[+] disk: %lu devices found\n", disks->length);

	for(size_t i = 0; i < disks->length; i++) {
		rtinfo_disk_dev_t *dev = disks->dev[i];
		printf("[+] disk: %s: sector size: %ld bytes\n", dev->name, dev->sectorsize);

		for(size_t s = 0; s < dev->sensors.length; s++) {
			rtinfo_disk_sensor_t *sensor = dev->sensors.sensors[s];

			printf("[+] disk: %s: sensors %d: [%s] %d\n", dev->name, sensor->id, sensor->label, sensor->critical);
		}
	}

	if(!(pwr = rtinfo_init_rapl_intel())) {
		printf("[-] intel rapl failed, disabled\n");
	}

	printf("[.] --------------------------------------\n");
	printf("[+] first (initial) metrics reading pass\n");
	printf("[.] --------------------------------------\n");
	rtinfo_get_cpu(cpu);
	rtinfo_get_network(net);
	rtinfo_get_disk(disks);
	rtinfo_get_rapl_intel(pwr);

	float waiting = interval / 1000.0;

	for(int a = 0; a < 5; a++) {
		printf("[+] waiting for ~%.2f seconds\n", waiting);
		usleep(interval * 1000);

		/* Reading CPU */
		printf("[.] --------------------------------------\n");
		printf("[+] second metrics reading pass\n");
		printf("[.] --------------------------------------\n");
		rtinfo_get_cpu(cpu);
		rtinfo_get_network(net);
		rtinfo_get_disk(disks);
		rtinfo_get_rapl_intel(pwr);

		rtinfo_mk_network_usage(net);
		rtinfo_mk_disk_usage(disks);
		rtinfo_mk_cpu_usage(cpu);
		rtinfo_mk_rapl_usage(pwr);

		printf("[:] \n");
		printf("[:] == DATA DUMP ==\n");
		printf("[:] \n");

		for(size_t i = 0; i < cpu->dev_length; i++) {
			printf("[ ] cpu: load: %2lu: %3u%%\n", i, cpu->dev[i].usage);
		}

		for(size_t i = 0; i < cpu->sensors_length; i++) {
			rtinfo_cpu_sensor_t *sensor = &cpu->sensors[i];
			printf("[ ] cpu: sensors %s: %d / %d\n", sensor->label, sensor->input, sensor->critical);
		}

		for(size_t i = 0; i < disks->length; i++) {
			rtinfo_disk_dev_t *dev = disks->dev[i];

			printf("[ ] disk: %-10s: read:  %6.2f MB/s [%5lu MB read]\n", dev->name, MBf(dev->read_speed), MB(dev->current.read));
			printf("[ ] disk: %-10s: write: %6.2f MB/s [%5lu MB written]\n", dev->name, MBf(dev->write_speed), MB(dev->current.written));

			for(size_t s = 0; s < dev->sensors.length; s++) {
				rtinfo_disk_sensor_t *sensor = dev->sensors.sensors[s];

				float input = (sensor->input / 1000.0);
				float criti = (sensor->critical / 1000.0);
				const char *xlabel = (sensor->label) ? sensor->label : "---";

				printf("[+] disk: %-10s: sensors %d: [%-10s] % 5.1f°C (Crit. % 5.1f°C)\n", dev->name, sensor->id, xlabel, input, criti);
			}

			printf("[+] disk: ---------------------\n");
		}


		for(size_t i = 0; i < pwr->zones; i++) {
			printf("[ ] intel repl pkg %lu: power: %f watt\n", i, pwr->pkgs[i]);
		}

		for(size_t i = 0; i < net->length; i++) {
			rtinfo_network_dev_t *dev = net->dev[i];

			printf("[+] net: %-15s", dev->name);
			printf("| %-18s | %-15s ", dev->hwaddr, dev->ip);
			printf("| %8.2f KB/s | %8.2f KB/s", (dev->tx_rate / 1024.0), (dev->rx_rate / 1024.0));
			printf("| Speed: %5d Mbps, MTU: %d\n",  dev->speed, dev->mtu);
		}

		/* Reading Memory */
		if(!rtinfo_get_memory(&memory))
			return 1;

		// memory is in KB
		printf("[+] memory: ---------------------\n");
		printf("[+] memory: ram:  %.2f MB (used: %.2f MB)\n", (memory.ram_total / 1024.0), (memory.ram_used / 1024.0));
		printf("[+] memory: swap: %.2f MB (free: %.2f MB)\n", (memory.swap_total / 1024.0), (memory.swap_free / 1024.0));

		/* Reading Load Average */
		if(!rtinfo_get_loadavg(&loadavg))
			return 1;

		printf("[+] loadavg: ---------------------\n");
		printf("[+] loadavg: %.2f, %.2f, %.2f\n", loadavg.load[0], loadavg.load[1], loadavg.load[2]);

		/* Reading Battery State */
		/* Use 'NULL' to search dynamically battery present */
		if(!rtinfo_get_battery(&battery, NULL))
			return 1;

		printf("[+] battery: ---------------------\n");
		printf("[+] battery: load: %d\n", battery.load);
		printf("[+] battery: full: %u / charge: %u / status: %d\n", battery.charge_now, battery.charge_full, battery.status);

		if(!rtinfo_get_uptime(&uptime))
			return 1;

		printf("[+] uptime: ---------------------\n");
		printf("[+] uptime: %u seconds\n", uptime.uptime);

		/* Reading Time Info */
		timeinfo = rtinfo_get_time();

		printf("[+] systime: ---------------------\n");
		printf("[ ] systime: %02d:%02d:%02d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

		//////////////////////////////////

		msgpack_sbuffer sbuf;
		msgpack_sbuffer_init(&sbuf);

		msgpack_packer pk;
		msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

		msgpack_pack_array(&pk, 11);

		// 1. Message type (10 = INIT, 20 = METRICS)
		msgpack_pack_uint8(&pk, 20);

		char xname[32];
		gethostname(xname, sizeof(xname));

		// 2. Hostname
		msgpack_pack_str(&pk, strlen(xname));
		msgpack_pack_str_body(&pk, xname, strlen(xname));

		// 3. Client Info/Version
		msgpack_pack_array(&pk, 2);
		msgpack_pack_uint32(&pk, reqid);
		msgpack_pack_float(&pk, rtinfo_version());

		// 4. Memory
		msgpack_pack_array(&pk, 4);
		msgpack_pack_uint64(&pk, memory.ram_total);
		msgpack_pack_uint64(&pk, memory.ram_used);
		msgpack_pack_uint64(&pk, memory.swap_total);
		msgpack_pack_uint64(&pk, memory.swap_free);

		// 5. Load average
		msgpack_pack_array(&pk, 3);
		msgpack_pack_uint32(&pk, (int32_t)(loadavg.load[0] * 1000));
		msgpack_pack_uint32(&pk, (int32_t)(loadavg.load[1] * 1000));
		msgpack_pack_uint32(&pk, (int32_t)(loadavg.load[2] * 1000));

		// 6. Battery
		msgpack_pack_array(&pk, 4);
		msgpack_pack_uint8(&pk, battery.status);
		msgpack_pack_uint8(&pk, battery.load);
		msgpack_pack_uint32(&pk, battery.charge_full);
		msgpack_pack_uint32(&pk, battery.charge_now);

		// 7. Time and Uptime
		msgpack_pack_array(&pk, 2);
		time_t now;
		time(&now);

		msgpack_pack_uint32(&pk, uptime.uptime);
		msgpack_pack_uint64(&pk, now);

		// 8. Intel REPL
		msgpack_pack_array(&pk, pwr->zones);
		for(size_t i = 0; i < pwr->zones; i++) {
			msgpack_pack_int32(&pk, (int32_t)(pwr->pkgs[i] * 1000));
		}

		// 9. CPU
		msgpack_pack_array(&pk, 2);

		// 9.1 CPU Usage
		msgpack_pack_array(&pk, cpu->dev_length);
		for(size_t i = 0; i < cpu->dev_length; i++) {
			msgpack_pack_uint8(&pk, cpu->dev[i].usage);
		}

		// 9.2 CPU Temperatures
		msgpack_pack_array(&pk, cpu->sensors_length);
		for(size_t i = 0; i < cpu->sensors_length; i++) {
			rtinfo_cpu_sensor_t *sensor = &cpu->sensors[i];

			msgpack_pack_array(&pk, 3);

			// 9.2.1 Sensor Name
			msgpack_pack_str(&pk, strlen(sensor->label));
			msgpack_pack_str_body(&pk, sensor->label, strlen(sensor->label));

			// 9.2.2 Temperature
			msgpack_pack_int32(&pk, sensor->input);

			// 9.2.3 Critical Temperature Threshold
			msgpack_pack_int32(&pk, sensor->critical);
		}

		// 10. Disks
		msgpack_pack_array(&pk, disks->length);
		for(size_t i = 0; i < disks->length; i++) {
			rtinfo_disk_dev_t *dev = disks->dev[i];

			msgpack_pack_array(&pk, 7);

			// 10.1 Name
			msgpack_pack_str(&pk, strlen(dev->name));
			msgpack_pack_str_body(&pk, dev->name, strlen(dev->name));

			// 10.2 Read Speed
			msgpack_pack_uint64(&pk, dev->read_speed);

			// 10.3 Write Speed
			msgpack_pack_uint64(&pk, dev->write_speed);

			// 10.4 Sectors Read
			msgpack_pack_uint64(&pk, dev->current.read);

			// 10.5 Sectors Written
			msgpack_pack_uint64(&pk, dev->current.written);

			// 10.6 IOPS
			msgpack_pack_int32(&pk, dev->iops);

			// 10.7 Sensors
			msgpack_pack_array(&pk, dev->sensors.length);
			for(size_t s = 0; s < dev->sensors.length; s++) {
				rtinfo_disk_sensor_t *sensor = dev->sensors.sensors[s];

				msgpack_pack_array(&pk, 3);

				// 10.7.1 Name
				msgpack_pack_str(&pk, strlen(sensor->label));
				msgpack_pack_str_body(&pk, sensor->label, strlen(sensor->label));

				// 10.7.2 Temperature
				msgpack_pack_int32(&pk, sensor->input);

				// 10.7.3 Critical
				msgpack_pack_int32(&pk, sensor->critical);
			}
		}

		// 11. Network
		msgpack_pack_array(&pk, net->length);
		for(size_t i = 0; i < net->length; i++) {
			rtinfo_network_dev_t *intf = net->dev[i];

			msgpack_pack_array(&pk, 7);

			// 11.1 Interface name
			msgpack_pack_str(&pk, strlen(intf->name));
			msgpack_pack_str_body(&pk, intf->name, strlen(intf->name));

			// 11.2 IPv4
			msgpack_pack_uint32(&pk, intf->addrv4);

			// 11.3 Current RX Usage
			msgpack_pack_uint64(&pk, intf->rx_rate);

			// 11.4 Current TX Usage
			msgpack_pack_uint64(&pk, intf->tx_rate);

			// 11.5 Total RX (bytes)
			msgpack_pack_uint64(&pk, intf->current.rx_bytes);

			// 11.6 Total TX (bytes)
			msgpack_pack_uint64(&pk, intf->current.tx_bytes);

			// 11.7 Link Speed
			msgpack_pack_uint16(&pk, intf->speed);
		}

		//-------------------------------------

		printf("\n\n----------------------\n\n");
		printf("Serialized: %lu bytes\n\n", sbuf.size);

		for(size_t x = 0; x < sbuf.size; x++) {
			printf("%02x", sbuf.data[x] & 0xff);
		}

		netinfo_send_packed(client->sockfd, sbuf.data, sbuf.size, &client->remote);

		printf("\n\n----------------------\n\n");

		msgpack_sbuffer_destroy(&sbuf);
	}

	////////////////////////////////////

	/* You should close your while here */
	rtinfo_free_cpu(cpu);
	rtinfo_free_network(net);
	rtinfo_free_disk(disks);
	rtinfo_free_rapl(pwr);

	return 0;
}

int networkside(char *server, int port, int interval, char **xdisks, size_t sdisks) {
	rtinfo_client_t client;

	printf("[+] starting rtinfo network client (version %.3f)\n", CLIENT_VERSION);
	printf("[ ] librtinfo version      : %.2f\n", rtinfo_version());

	if(server_validate(server, port, &client)) {
		// server rejected, stopping
		return 1;
	}

	int ret = client_process(&client, interval, xdisks, sdisks);

	return ret;
}
