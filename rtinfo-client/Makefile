include config.mk

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

ifdef STATIC
	CFLAGS += -march=nocona
	LDFLAGS += -static
endif

ifdef SystemRoot
	# Windows
	RM   = del /Q
	EXEC := $(EXEC).exe
else
	RM  = rm -fv
	
	ifneq ($(wildcard /usr/include/endian.h),)
		CFLAGS += -DHAVE_ENDIAN_H
	endif
endif

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
	$(RM) *.o

mrproper: clean
	$(RM) $(EXEC)

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
