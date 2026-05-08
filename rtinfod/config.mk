include ../config.mk

EXEC = rtinfod

CFLAGS  += -pthread
LDFLAGS += -lrtinfo -pthread -lm -ljansson
