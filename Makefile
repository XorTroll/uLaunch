
export Q_VERSION := 0.2

.PHONY: all dev clean

all:
	@$(MAKE) -C master-libnx/
	@$(MAKE) -C libstratosphere/
	@$(MAKE) -C SystemAppletQDaemon/
	@$(MAKE) -C LibraryAppletQMenu/
	@$(MAKE) -C LibraryAppletQHbTarget/
	@$(MAKE) -C SystemApplicationQHbTarget/
	@rm -rf $(CURDIR)/SdOut
	@mkdir -p $(CURDIR)/SdOut/atmosphere/contents
	@mkdir -p $(CURDIR)/SdOut/ulaunch/bin/QMenu
	@cp -r $(CURDIR)/SystemAppletQDaemon/Out $(CURDIR)/SdOut/atmosphere/contents/0100000000001000
	@cp -r $(CURDIR)/LibraryAppletQMenu/Out $(CURDIR)/SdOut/atmosphere/contents/010000000000100B
	@build_romfs $(CURDIR)/LibraryAppletQMenu/RomFs $(CURDIR)/SdOut/ulaunch/bin/QMenu/romfs.bin
	@cp -r $(CURDIR)/LibraryAppletQHbTarget/Out $(CURDIR)/SdOut/atmosphere/contents/0100000000001001
	@cp -r $(CURDIR)/SystemApplicationQHbTarget/Out $(CURDIR)/SdOut/atmosphere/contents/01008BB00013C000

setdev:
	$(eval export Q_DEV := 1)
	@echo
	@echo IMPORTANT! Building in development mode - do not treat this build as release...
	@echo

dev: setdev all

clean:
	@rm -rf $(CURDIR)/SdOut
	@$(MAKE) clean -C master-libnx/
	@$(MAKE) clean -C libstratosphere/
	@$(MAKE) clean -C SystemAppletQDaemon/
	@$(MAKE) clean -C LibraryAppletQMenu/
	@$(MAKE) clean -C LibraryAppletQHbTarget/
	@$(MAKE) clean -C SystemApplicationQHbTarget/