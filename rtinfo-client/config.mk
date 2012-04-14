EXEC = rtinfo-client

# flags
VERSION = 6.0
CFLAGS  = -W -Wall -O2 -pipe -ansi -pedantic -std=gnu99 -DCLIENT_VERSION=$(VERSION)
LDFLAGS = -lrtinfo -lncurses

# CFLAGS  += "-I../../librtinfo/"
# LDFLAGS += "-L../../librtinfo/"
# CC = cc

