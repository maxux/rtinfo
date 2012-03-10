EXEC = rtinfo-server

# flags
CFLAGS  = -W -Wall -O2 -pipe -ansi -pedantic -std=gnu99 -I../../librtinfo/
# LDFLAGS = ../librtinfo/sysinfo.o ../librtinfo/misc.o -lncurses
LDFLAGS = -L../../librtinfo/ -lrtinfo -lncurses -lpthread -lm

# CC = cc

