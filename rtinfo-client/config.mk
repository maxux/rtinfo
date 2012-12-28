EXEC = rtinfo-client

VERSION = 103
CFLAGS  = -W -Wall -O2 -pipe -ansi -std=gnu99 -DCLIENT_VERSION=$(VERSION)
LDFLAGS = -lrtinfo

CC = gcc
