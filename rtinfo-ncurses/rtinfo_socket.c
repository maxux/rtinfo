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
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "rtinfo_ncurses.h"
#include "rtinfo_socket.h"
#include "rtinfo_client_data.h"
#include "rtinfo_display.h"

char * working(int sockfd) {
	char buffer[256], *match, *data;
	int length, alloc, missing;
	
	strcpy(buffer, "GET /json HTTP/1.0\r\n\r\n");
	if(send(sockfd, buffer, strlen(buffer), 0) < 0) {
		display_perror("send");
		return NULL;
	}
	
	/* reading header */
	if((length = recv(sockfd, buffer, sizeof(buffer), 0)) < 0) {
		display_perror("read");
		return NULL;
	}
	
	buffer[length] = '\0';
	
	/* checking length */
	if(!(match = strstr(buffer, "Content-length: "))) {
		display_error("Cannot find Content-length\n");
		return NULL;
	}

	/* Alloc is content-length + null */
	alloc = atol(match + 16) + 1;
	
	if(!(match = strstr(buffer, "\r\n\r\n"))) {
		display_error("Cannot find start of data\n");
		return NULL;
	}
	
	if(!(data = (char *) malloc(sizeof(char) * alloc))) {
		display_perror("malloc");
		return NULL;
	}
	
	/* copy current data from buffer */
	length  = snprintf(data, alloc, "%s", match + 4);	
	missing = alloc - length;

	/* reading missing data */
	while(strlen(data) != (unsigned int) alloc - 1) {
		/* ending read */
		if((length = recv(sockfd, data + alloc - missing, missing, 0)) < 0)
			display_perror("read");
		
		data[alloc - missing + length] = '\0';
		missing -= length;
	}
	
	close(sockfd);
	
	return data;
}

char * socket_rtinfo(char *server, int port) {
	int sockfd;
	struct sockaddr_in addr_remote;
	struct hostent *hent;

	/* Creating client socket */
	addr_remote.sin_family      = AF_INET;
	addr_remote.sin_port        = htons(port);
	
	/* dns resolution */
	if((hent = gethostbyname(server)) == NULL) {
		display_perror("gethostbyname");
		return NULL;
	}
		
	memcpy(&addr_remote.sin_addr, hent->h_addr_list[0], hent->h_length);
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		display_perror("socket");
		return NULL;
	}

	/* connecting */
	if(connect(sockfd, (const struct sockaddr *) &addr_remote, sizeof(addr_remote)) < 0) {
		display_perror("connect");
		return NULL;
	}

	return working(sockfd);
}

