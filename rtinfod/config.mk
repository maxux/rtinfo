EXEC = rtinfod

VERSION = 0.1
CFLAGS  = -W -Wall -O2 -pipe -ansi -std=gnu99 -pthread -g
LDFLAGS = -lrtinfo -pthread -lm -ljansson

CC = gcc
