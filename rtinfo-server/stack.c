#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>
#include <pthread.h>
#include "server.h"
#include "stack.h"
#include "../rtinfo-common/socket.h"
#include "display.h"

extern client_t *clients;
extern int nbclients;

client_t * stack_client(client_t *new) {
	new->next = clients;
	clients = new;
	
	return new;
}

client_t * unstack_client(client_t *remove) {
	client_t *temp, *prev = NULL;
	
	temp = clients;
	while(temp && (remove->id != temp->id)) {
		prev = temp;
		temp = temp->next;
	}
	
	if(temp) {
		prev = temp->next;
		free(temp);
		
	} else return NULL;
	
	return clients;
}

client_t * stack_search(char *name) {
	client_t *temp;
	int line = 0;
	
	temp = clients;
	while(temp && strcmp(temp->name, name)) {
		line += temp->nbiface;
		temp = temp->next;
	}
	
	if(temp)
		temp->line = line;
		
	return temp;
}

void * stack_ping(void *dummy) {
	client_t *temp;
	time_t t;
	int i;
	struct timespec tv;
	
	/* Loop each 5 seconds */
	tv.tv_sec = 5;
	tv.tv_nsec = 0;
	
	while(1) {
		time(&t);
		temp = clients;
		
		while(temp) {
			if(t - 30 > temp->last) {
				/* Locking screen */
				pthread_mutex_lock(&mutex_screen);
				
				move(temp->id + 2, 0);
				
				attrset(A_BOLD | COLOR_PAIR(4));
				
				printw(" %-14s ", temp->name);
				
				move(nbclients + 5 + temp->line, 0);
		
				for(i = 0; i < temp->nbiface; i++) {
					printw(" %-14s", temp->name);
					move(nbclients + 5 + temp->line + i, 0);
				}
					
				attrset(COLOR_PAIR(1));
				refresh();
				
				/* Unlocking screen */
				pthread_mutex_unlock(&mutex_screen);
			}
			
			temp = temp->next;
		}
		
		nanosleep(&tv, &tv);
	}
	
	return dummy;
}
