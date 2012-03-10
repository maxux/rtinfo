EXEC = rtinfo-server

# flags
VERSION = 4.0
CFLAGS  = -W -Wall -O2 -pipe -ansi -pedantic -std=gnu99 -I../../librtinfo/ -DSERVER_VERSION=$(VERSION)
# LDFLAGS = ../librtinfo/sysinfo.o ../librtinfo/misc.o -lncurses
LDFLAGS = -L../../librtinfo/ -lrtinfo -lncurses -lpthread -lm

# CC = cc

