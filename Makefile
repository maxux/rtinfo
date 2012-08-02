all: client server local

client:
	cd rtinfo-client && $(MAKE)
	
server:
	cd rtinfo-server && $(MAKE)

local:
	cd rtinfo-local && $(MAKE)

clean:
	cd rtinfo-client && $(MAKE) clean
	cd rtinfo-server && $(MAKE) clean
	cd rtinfo-local && $(MAKE) clean

mrproper: clean
	cd rtinfo-client && $(MAKE) mrproper
	cd rtinfo-server && $(MAKE) mrproper
	cd rtinfo-local && $(MAKE) mrproper
