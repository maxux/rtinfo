include config.mk

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

