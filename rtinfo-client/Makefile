include ../config.mk

EXEC = rtinfo-client
PREFIX = /usr/local/
CFLAGS += -W -Wall -O2 -pipe -ansi -std=gnu99
LDFLAGS += -lrtinfo

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

ifdef STATIC
	CFLAGS += -march=nocona
	LDFLAGS += -static
endif

ifneq ($(wildcard /usr/include/endian.h),)
	CFLAGS += -DHAVE_ENDIAN_H
endif

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o

mrproper: clean
	rm -f $(EXEC)

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
	rm -vf $(DESTDIR)$(PREFIX)bin/$(EXEC)
	rm -vf $(DESTDIR)/etc/init.d/$(EXEC)
	rm -vf $(DESTDIR)/etc/conf.d/$(EXEC)
