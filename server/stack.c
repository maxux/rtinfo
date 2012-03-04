#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>
#include "../socket.h"
#include "server.h"
#include "stack.h"

extern client_t *clients;

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
	
	temp = clients;
	while(temp && strcmp(temp->name, name))
		temp = temp->next;
	
	return temp;
}

void stack_ping() {
	client_t *temp = clients;
	time_t t;
	
	time(&t);
	
	while(temp) {
		if(t - 30 > temp->last) {
			move(temp->id, 0);
			clrtoeol();
			attrset(A_BOLD | COLOR_PAIR(4));
			printw(" %-10s | Ping timeout", temp->name);
			attrset(COLOR_PAIR(1));	
		}
		
		temp = temp->next;
	}
}
