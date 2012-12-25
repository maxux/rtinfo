all: client server

client:
	cd rtinfo-client && $(MAKE)
	
server:
	cd rtinfo-server && $(MAKE)

clean:
	cd rtinfo-client && $(MAKE) clean
	cd rtinfo-server && $(MAKE) clean

mrproper: clean
	cd rtinfo-client && $(MAKE) mrproper
	cd rtinfo-server && $(MAKE) mrproper
