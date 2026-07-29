/*
 * rtinfod is the daemon monitoring rtinfo remote clients
 * Copyright (C) 2012  DANIEL Maxime <root@maxux.net>
 *
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include <time.h>
#include <rtinfo.h>

#include <msgpack.h>

#include "rtinfod.h"
#include "rtinfod_stack.h"
#include "rtinfod_ip.h"
#include "rtinfod_input.h"
#include "rtinfod_debug.h"
#include "rtinfod_output.h"

char *__bind_input[MAX_NETWORK_BIND] = {"0.0.0.0"};
int __bind_input_count = 1;

void *init_input(void *data) {
	thread_input_t *root = (thread_input_t *) data;
	thread_input_t *new[MAX_NETWORK_BIND];
	int i;

	for(i = 0; i < __bind_input_count && __bind_input[i]; i++) {
		if(!(new[i] = (thread_input_t *) malloc(sizeof(thread_input_t))))
			diep("[-] input: malloc");

		new[i]->port = root->port;

		bzero(&new[i]->si_me, sizeof(new[i]->si_me));
		verbose("[+] input: binding to [%s], port: %d\n", __bind_input[i], root->port);

		new[i]->si_me.sin_family      = AF_INET;
		new[i]->si_me.sin_port        = htons(root->port);
		new[i]->si_me.sin_addr.s_addr = inet_addr(__bind_input[i]);

		if((new[i]->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
			diep("[-] input: socket");

		if(bind(new[i]->sockfd, (struct sockaddr*) &new[i]->si_me, sizeof(new[i]->si_me)) == -1)
			diep("[-] input: bind");

		if(pthread_create(&new[i]->thread, NULL, thread_input, (void *) new[i]))
			diep("[-] input: pthread_create");
	}

	for(i = 0; i < __bind_input_count && __bind_input[i]; i++)
		pthread_join(new[i]->thread, NULL);

	return data;
}

typedef struct rtinfo_client_t {
	int sockfd;
	struct sockaddr_in remote;
	socklen_t slen;

} rtinfo_client_t;

int input_client_init(msgpack_object_array *root, rtinfo_client_t *client) {
	if(root->ptr[2].type != MSGPACK_OBJECT_ARRAY) {
		verbose("[+] input: init: version descriptor malformed\n");
		return 1;
	}

	msgpack_object_array *ver = &root->ptr[2].via.array;
	float libclient = ver->ptr[1].via.f64;
	int response = 11; // OK
	int confirm = 0;

	verbose("[+] input: init: client version: %f\n", libclient);

	if(libclient < 5 || libclient >= 6) {
		verbose("[+] input: init: client rejected\n");

		response = 12; // REJECT
		confirm = 1;
	}

	msgpack_sbuffer sbuf;
	msgpack_sbuffer_init(&sbuf);

	msgpack_packer pk;
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

	msgpack_pack_array(&pk, 3);

	// 1. Message type (10 = INIT, 11 = OK, 12 = REJECT, 20 = METRICS)
	msgpack_pack_uint8(&pk, response);

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

	if(sendto(client->sockfd, sbuf.data, sbuf.size, 0, (const struct sockaddr *) &client->remote, sizeof(struct sockaddr_in)) == -1)
		perror("[-] input: sendto");

	msgpack_sbuffer_destroy(&sbuf);

	return confirm;
}

int input_client_metrics(msgpack_object_array *root, rtinfo_client_t *client) {

	return 0;
}

int input_parse(uint8_t *buffer, size_t recvsize, rtinfo_client_t *client) {
	int confirm = 0;

	msgpack_unpacker *unp = msgpack_unpacker_new(1024);
	if(msgpack_unpacker_buffer_capacity(unp) < recvsize) {
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
			verbose("[+] input: message unpacked (type: %d)\n", obj.type);

			printf("[.] -----------------------------\n");
			msgpack_object_print(stdout, und.data);
			printf("\n[.] -----------------------------\n");

			if(obj.type != MSGPACK_OBJECT_ARRAY) {
				verbose("[+] input: unpacked message is not an array\n");
				confirm = 4;
				break;
			}

			if(obj.via.array.size < 3) {
				verbose("[+] input: unpacked array is too small (size: %u)\n", obj.via.array.size);
				confirm = 5;
				break;
			}

			if(obj.via.array.ptr[0].type != MSGPACK_OBJECT_POSITIVE_INTEGER) {
				verbose("[+] input: invalid packet identifier format (type: %d)\n", obj.via.array.ptr[0].type);
				confirm = 6;
				break;
			}

			// INIT
			if(obj.via.array.ptr[0].via.u64 == 10) {
				confirm = input_client_init(&obj.via.array, client);
				break;
			}

			// METRICS
			if(obj.via.array.ptr[0].via.u64 == 20) {
				confirm = input_client_metrics(&obj.via.array, client);
				break;
			}
		}
		break;

	case MSGPACK_UNPACK_CONTINUE:
		printf("CONTINUE\n");
		confirm = 3;
		/* cheking capacity, reserve buffer, copy additional data to the buffer, */
		/* notify consumed buffer size, then call msgpack_unpacker_next(&unp, &und) again */
		break;

	case MSGPACK_UNPACK_PARSE_ERROR:
		printf("PARSE ERROR\n");
		confirm = 2;
		break;
	}

	msgpack_unpacked_destroy(&und);
	msgpack_unpacker_free(unp);

	return confirm;
}

void *thread_input(void *data) {
	struct sockaddr_in remote;
	int recvsize, sockfd;
	size_t checklength;
	client_t *xclient;
	thread_input_t *thread_input = (thread_input_t*) data;
	uint32_t index;

	void *buffer = malloc(sizeof(char) * BUFFER_SIZE);

	// FIXME
	/* ip allowed */
	unsigned int *mask = NULL, *baseip = NULL;
	int nballow = 0, i;

	verbose("[+] input: ready, waiting data\n");
	sockfd = thread_input->sockfd; // FIXME

	rtinfo_client_t client = {
		.sockfd = thread_input->sockfd,
		.slen = sizeof(remote),
	};

	while(1) {
		if((recvsize = recvfrom(client.sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client.remote, &client.slen)) == -1)
			perror("recvfrom");

		verbose("[+] input: read %u bytes from %s\n", recvsize, inet_ntoa(client.remote.sin_addr));

		if(recvsize < 4) {
			verbose("[+] input: skipping packet too small\n");
			continue;
		}

		if(global.debug)
			debug_input(buffer, recvsize);

		int parser = input_parse(buffer, recvsize, &client);
		printf(">>> PARSER: %d\n", parser);


		#if 0

		/* Checking ip --allowed */
		if(nballow) {
			for(i = 0; i < nballow; i++) {
				if((remote.sin_addr.s_addr & mask[i]) == baseip[i])
					i = -1;
			}

			/* ip disallowed */
			if(i != -1) {
				verbose("Warning: denied packed received from: %s\n", inet_ntoa(remote.sin_addr));
				continue;
			}
		}

		/* Converting header endianess */
		packed = (netinfo_packed_t*) buffer;
		convert_header(packed);

		/* Locking globals */
		pthread_mutex_lock(&global.mutex_clients);

		/* Reading header */
		if(packed->options & QRY_SOCKET) {
			verbose("[+] input: new client: %s/%s\n", inet_ntoa(remote.sin_addr), packed->hostname);

			/* Authentification accepted */
			packed->options  = htobe32(ACK_SOCKET);

			/* Sending Server Version */
			packed->version  = htobe32((int)(SERVER_VERSION));

			if(sendto(sockfd, buffer, recvsize, 0, (const struct sockaddr *) &remote, sizeof(struct sockaddr_in)) == -1)
				perror("[-] input: sendto");

			/* Checking if the client is not already on stack */
			if(!(client = stack_search(packed->hostname)))
				client = stack_newclient(packed->hostname, remote.sin_addr.s_addr);

			goto unlock;
		}

		/* Version Check */
		if(packed->version != (unsigned int) (SERVER_VERSION)) {
			warning("[-] input: warning: invalid version (%u) from %s (%s)\n", packed->version, inet_ntoa(remote.sin_addr), packed->hostname);
			goto unlock;
		}

		/* Checking size consistency */
		if(packed->options == USE_NETWORK) {
			/* Network size, should not exceed 32 bytes per interface name */
			net = (netinfo_packed_net_t*) packed;
			net->nbiface = be32toh(net->nbiface);

			checklength = sizeof(netinfo_packed_net_t) + (sizeof(rtinfo_network_legacy_t) * net->nbiface) + (net->nbiface * 32);

			if((size_t) recvsize > checklength) {
				warning("[-] input: warning: wrong network summary datasize (%d bytes) from %s, should be <= %zu bytes.\n",
					recvsize, inet_ntoa(remote.sin_addr), checklength);

				goto unlock;
			}

		} else if(packed->options == USE_SUMMARY) {
			/* Summary size */
			checklength = sizeof(netinfo_packed_t) + (sizeof(rtinfo_cpu_legacy_t) * be32toh(packed->nbcpu));

			if((size_t) recvsize != checklength) {
				warning("[-] input: warning: wrong summary datasize (%d bytes) from %s, should be %zu bytes.\n",
					recvsize, inet_ntoa(remote.sin_addr), checklength);

				goto unlock;
			}

		} else if(packed->options == USE_DISK) {
			/* Disk size, should not exceed 32 bytes per disk name */
			disk = (netinfo_packed_disk_t *) packed;
			disk->nbdisk = be32toh(disk->nbdisk);

			checklength = sizeof(netinfo_packed_disk_t) + (sizeof(rtinfo_disk_legacy_t) * disk->nbdisk) + (disk->nbdisk * 32);

			if((size_t) recvsize > checklength) {
				warning("[-] input: warning: wrong disk datasize (%d bytes) from %s, should be <= %zu bytes.\n",
					recvsize, inet_ntoa(remote.sin_addr), checklength);

				goto unlock;
			}
		}

		/* Searching client on stack */
		if(!(client = stack_search(packed->hostname)))
			client = stack_newclient(packed->hostname, remote.sin_addr.s_addr);

		/* Saving last update time (timeout) */
		time(&client->lasttime);

		if(packed->options == USE_NETWORK) {
			/* Updating allocation */
			if(client->net_length != (size_t) recvsize) {
				client->net = (netinfo_packed_net_t *) realloc(client->net, recvsize);
				client->net_length = (size_t) recvsize;
			}

			memcpy(client->net, buffer, recvsize);

			client->net->nbiface = htobe32(client->net->nbiface);
			read = client->net->net;

			for(index = 0; index < client->net->nbiface; index++) {
				convert_packed_net(read);
				read = (rtinfo_network_legacy_t*) ((char*) read + sizeof(rtinfo_network_legacy_t) + read->name_length);
			}

		} else if(packed->options == USE_SUMMARY) {
			/* Updating allocation */
			if(client->summary_length != (size_t) recvsize) {
				client->summary = (netinfo_packed_t *) realloc(client->summary, recvsize);
				client->summary_length = recvsize;
			}

			/* Copy current data */
			memcpy(client->summary, buffer, recvsize);

			/* Correct indian */
			convert_packed(client->summary);

		} else if(packed->options == USE_DISK) {
			/* Updating allocation */
			if(client->disk_length != (size_t) recvsize) {
				client->disk = (netinfo_packed_disk_t *) realloc(client->disk, recvsize);
				client->disk_length = (size_t) recvsize;
			}

			memcpy(client->disk, buffer, recvsize);

			client->disk->nbdisk = htobe32(client->disk->nbdisk);
			dev = client->disk->disk;

			for(index = 0; index < client->disk->nbdisk; index++) {
				convert_packed_disk(dev);
				dev = (rtinfo_disk_legacy_t *) ((char *) dev + sizeof(rtinfo_disk_legacy_t) + dev->name_length);
			}
		}

		/* Unlocking globals */
		unlock:
			pthread_mutex_unlock(&global.mutex_clients);

		#endif
	}

	close(sockfd);

	return data;
}
