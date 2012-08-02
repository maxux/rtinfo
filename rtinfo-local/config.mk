EXEC = rtinfo-local

VERSION = 6.0
CFLAGS  = -W -Wall -O2 -pipe -ansi -pedantic -std=gnu99 -DLOCAL_VERSION=$(VERSION)
LDFLAGS = -lrtinfo -lncurses
