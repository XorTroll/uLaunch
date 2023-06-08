
VERSION_MAJOR	:=	1
VERSION_MINOR	:=	0
VERSION_MICRO	:=	0
VERSION			:=	$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)

DEFAULT_HB_NAME		:=	Unnamed homebrew
DEFAULT_HB_AUTHOR	:=	Unknown author
DEFAULT_HB_VERSION	:=	Unknown version

export CPP_DEFS	:=	-DUL_VERSION=\"$(VERSION)\"

.PHONY: all fresh clean default_hb usystem uloader umenu

all: default_hb usystem uloader umenu

fresh: clean all

clean:
	@$(MAKE) clean -C projects/uSystem
	@$(MAKE) clean -C projects/uLoader
	@$(MAKE) clean -C projects/uMenu
	@rm -rf SdOut

default_hb:
	@nacptool --create "$(DEFAULT_HB_NAME)" "$(DEFAULT_HB_AUTHOR)" "$(DEFAULT_HB_VERSION)" default_hb_nacp.nacp
	@mkdir -p SdOut/ulaunch
	@cp default_hb_nacp.nacp SdOut/ulaunch/default_hb_nacp.nacp
	@cp default_hb_icon.jpg SdOut/ulaunch/default_hb_icon.jpg

usystem:
	@$(MAKE) -C projects/uSystem
	@mkdir -p SdOut/atmosphere/contents/0100000000001000
	@cp projects/uSystem/out/nintendo_nx_arm64_armv8a/release/uSystem.nsp SdOut/atmosphere/contents/0100000000001000/exefs.nsp

uloader:
	@$(MAKE) -C projects/uLoader
	@mkdir -p SdOut/ulaunch/bin/uLoader
	@mkdir -p SdOut/ulaunch/bin/uLoader/applet
	@cp projects/uLoader/uLoader.nso SdOut/ulaunch/bin/uLoader/applet/main
	@cp projects/uLoader/uLoader_applet.npdm SdOut/ulaunch/bin/uLoader/applet/main.npdm
	@mkdir -p SdOut/ulaunch/bin/uLoader/application
	@cp projects/uLoader/uLoader.nso SdOut/ulaunch/bin/uLoader/application/main
	@cp projects/uLoader/uLoader_application.npdm SdOut/ulaunch/bin/uLoader/application/main.npdm

umenu:
	@$(MAKE) -C projects/uMenu
	@mkdir -p SdOut/ulaunch/bin/uMenu
	@cp projects/uMenu/uMenu.nso SdOut/ulaunch/bin/uMenu/main
	@cp projects/uMenu/uMenu.npdm SdOut/ulaunch/bin/uMenu/main.npdm
	@build_romfs projects/uMenu/romfs SdOut/ulaunch/bin/uMenu/romfs.bin
