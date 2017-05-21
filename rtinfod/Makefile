include config.mk

ifdef STATIC
	CFLAGS += -march=nocona
	LDFLAGS += -static
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
	$(CC) $(CFLAGS) -c $<

clean:
	rm -fv *.o

mrproper: clean
	rm -fv $(EXEC)

install:
	mkdir -p $(DESTDIR)$(PREFIX)bin
	cp -v $(EXEC) $(DESTDIR)$(PREFIX)bin/
ifeq ($(OPENRC), YES)
	mkdir -p $(DESTDIR)/etc/init.d
	cp -v initservices/openrc/init $(DESTDIR)/etc/init.d/$(EXEC)
	sed "s#__EXECPATH__#$(PREFIX)bin/$(EXEC)#" -i $(DESTDIR)/etc/init.d/$(EXEC)
	mkdir -p $(DESTDIR)/etc/conf.d
	cp -v initservices/openrc/conf $(DESTDIR)/etc/conf.d/$(EXEC)
endif

uninstall:
	rm -fv $(DESTDIR)$(PREFIX)bin/$(EXEC)
