
export Q_VERSION := dev

.PHONY: all clean

all:
	@$(MAKE) -C SystemAppletQDaemon/
	@$(MAKE) -C LibraryAppletQMenu/
	@$(MAKE) -C LibraryAppletQHbTarget/
	@$(MAKE) -C SystemApplicationQHbTarget/
	@rm -rf $(CURDIR)/SdOut
	@mkdir -p $(CURDIR)/SdOut/titles
	@cp -r $(CURDIR)/SystemAppletQDaemon/Out $(CURDIR)/SdOut/titles/0100000000001000
	@cp -r $(CURDIR)/LibraryAppletQMenu/Out $(CURDIR)/SdOut/titles/010000000000100B
	@cp -r $(CURDIR)/LibraryAppletQHbTarget/Out $(CURDIR)/SdOut/titles/0100000000001009
	@cp -r $(CURDIR)/SystemApplicationQHbTarget/Out $(CURDIR)/SdOut/titles/01008BB00013C000

clean:
	@rm -rf $(CURDIR)/SdOut
	@$(MAKE) clean -C SystemAppletQDaemon/
	@$(MAKE) clean -C LibraryAppletQMenu/
	@$(MAKE) clean -C LibraryAppletQHbTarget/
	@$(MAKE) clean -C SystemApplicationQHbTarget/