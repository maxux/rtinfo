EXEC = rtinfo-client

VERSION = 100
CFLAGS  = -W -Wall -O2 -pipe -ansi -pedantic -std=gnu99 -DCLIENT_VERSION=$(VERSION)
LDFLAGS = -lrtinfo
