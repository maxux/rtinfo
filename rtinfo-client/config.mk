EXEC = rtinfo-client

# flags
CFLAGS  = -W -Wall -O2 -pipe -ansi -pedantic -std=gnu99 -I../../librtinfo/
LDFLAGS = -L../../librtinfo/ -lrtinfo -lncurses

# CC = cc

