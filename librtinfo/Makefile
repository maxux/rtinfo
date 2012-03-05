include config.mk

# Finding gcc target
GCC = $(shell gcc -v 2>&1 | grep ^Target | cut -b 9- )

ifneq "$(GCC)" ""
    CC = $(GCC)-gcc
endif

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(OBJ)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -fv *.o
