clean:
	@$(MAKE) -C rtinfod $@
	@$(MAKE) -C rtinfo-client $@
	@$(MAKE) -C rtinfo-ncurses $@

mrproper:
	@$(MAKE) -C rtinfod $@
	@$(MAKE) -C rtinfo-client $@
	@$(MAKE) -C rtinfo-ncurses $@
