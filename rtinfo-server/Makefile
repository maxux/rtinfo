include config.mk

# Finding gcc target
GCC = $(shell gcc -v 2>&1 | grep ^Target | cut -b 9- )

ifneq "$(GCC)" ""
    CC = $(GCC)-gcc
endif

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: options $(EXEC)

options: config.mk
	@echo $(EXEC) build options:
	@echo "CFLAGS   = $(CFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"
	@echo "CC       = $(CC)"

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -fv *.o

mrproper: clean
	rm -fv $(EXEC)

