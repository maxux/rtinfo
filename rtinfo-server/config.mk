EXEC = rtinfo-server

VERSION = 101
CFLAGS  = -W -Wall -O2 -pipe -ansi -pedantic -std=gnu99 -DSERVER_VERSION=$(VERSION) -pthread
LDFLAGS = -lrtinfo -lncurses -pthread -lm
