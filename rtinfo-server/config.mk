EXEC = rtinfo-server

# flags
VERSION = 5.0
CFLAGS  = -W -Wall -O2 -pipe -ansi -pedantic -std=gnu99 -DSERVER_VERSION=$(VERSION) -pthread
LDFLAGS = -lrtinfo -lncurses -pthread -lm

# CFLAGS += "-I../../librtinfo"
# LDFLAGS += "-L../../librtinfo"

# CC = cc

