EXEC = rtinfo-client

# flags
VERSION = 4.0
CFLAGS  = -W -Wall -O2 -pipe -ansi -pedantic -std=gnu99 -I../../librtinfo/ -DCLIENT_VERSION=$(VERSION)
LDFLAGS = -L../../librtinfo/ -lrtinfo -lncurses

# CC = cc

