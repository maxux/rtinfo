#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>
#include <pthread.h>
#include "../rtinfo-common/socket.h"
#include "server.h"
#include "server_stack.h"
#include "server_display.h"

extern client_t *clients, *lastclient;
extern int nbclients;

client_t * stack_client(client_t *new) {
	/* Assume that the stack is newer empty */
	lastclient->next = new;
	lastclient = new;
	new->next = NULL;	
	
	return new;
}

client_t * unstack_client(client_t *remove) {
	client_t *temp;
	
	temp = clients;
	while(temp && (remove->id != temp->id))
		temp = temp->next;
	
	if(temp) {
		free(temp);
		
	} else return NULL;
	
	return clients;
}

client_t * stack_search(char *name) {
	client_t *temp;
	
	temp = clients;
	while(temp && strcmp(temp->name, name))
		temp = temp->next;
		
	return temp;
}

void stack_timeout(client_t *root, time_t timeout) {
	uint32_t i;
	
	while(root) {
		if(timeout > root->last) {
			// Locking screen
			pthread_mutex_lock(&mutex_screen);
			
			/* Summary */
			wmove(root->window, 0, 0);
			wattrset(root->window, A_BOLD | COLOR_PAIR(9));
			
			wprintw(root->window, " %-14s ", root->name);
			wrefresh(root->window);
			
			wattrset(root->window, COLOR_PAIR(1));
			
			/* Network */
			wattrset(root->netwindow, A_BOLD | COLOR_PAIR(9));

			for(i = 0; i < root->nbiface; i++) {
				wmove(root->netwindow, i, 0);
				wprintw(root->netwindow, " %-14s ", root->name);
			}				
			
			wrefresh(root->netwindow);
			wattrset(root->netwindow, COLOR_PAIR(1));
			
			// Unlocking screen
			pthread_mutex_unlock(&mutex_screen);
		}
		
		root = root->next;
	}
}

void * stack_ping(void *dummy) {
	time_t timeout;
	struct timespec tv;
	
	/* Loop each 5 seconds */
	tv.tv_sec = 5;
	tv.tv_nsec = 0;
	nanosleep(&tv, &tv);
	
	while(1) {
		timeout = time(NULL) - 30;
		stack_timeout(clients->next, timeout);		
		nanosleep(&tv, &tv);
	}
	
	return dummy;
}

client_t * stack_newclient(netinfo_packed_t *buffer, int clientid) {
	client_t *client;
	
	client = (client_t*) malloc(sizeof(client_t));
			
	client->id      = clientid;
	client->nbiface = 1;
	client->last    = time(NULL);
	
	strncpy(client->name, buffer->hostname, sizeof(client->name));
	client->name[sizeof(client->name) - 1] = '\0';
	
	// Windows positions
	client->winx      = 0;
	client->winy      = clientid + 2;
	client->window    = newwin(2, WINDOW_WIDTH, client->winy, 0);
	
	client->netx      = 0;
	client->nety      = lastclient->nety + lastclient->nbiface + 2;
	client->netwindow = newwin(2, WINDOW_WIDTH, client->nety, 0);
	
	// New line on summary, moving all under
	netmoveunder(clients, 1);
	
	// Add client on the stack, become the last client
	if(!stack_client(client)) {
		fprintf(stderr, "[-] stacking client failed\n");
		exit(EXIT_FAILURE);
	}
	
	joining(client);
	
	nbclients++;
	
	return client;
}
