all:
	cd librtinfo && $(MAKE)
	cd rtinfo && $(MAKE)
	cd rtinfo-server && $(MAKE)

clean:
	cd librtinfo && $(MAKE) clean
	cd rtinfo && $(MAKE) clean
	cd rtinfo-server && $(MAKE) clean

mrproper: clean
	cd rtinfo && $(MAKE) mrproper
	cd rtinfo-server && $(MAKE) mrproper
