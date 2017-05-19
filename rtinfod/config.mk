include ../config.mk

EXEC = rtinfod

VERSION = 0.1
CFLAGS  += -pthread
LDFLAGS += -lrtinfo -pthread -lm -ljansson
