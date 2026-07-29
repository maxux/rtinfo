include ../config.mk

EXEC = rtinfod

CFLAGS  += -pthread
LDFLAGS += ../../librtinfo-testing/linux/librtinfo.a -pthread -lm -ljansson -lmsgpack-c
