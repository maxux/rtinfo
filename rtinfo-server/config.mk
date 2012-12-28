EXEC = rtinfo-server

VERSION = 103
CFLAGS  = -W -Wall -O2 -pipe -ansi -std=gnu99 -DSERVER_VERSION=$(VERSION) -pthread -g
LDFLAGS = -lrtinfo -lncurses -pthread -lm

CC = gcc
