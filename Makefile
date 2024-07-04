VERSION_MAJOR	:=	1
VERSION_MINOR	:=	0
VERSION_MICRO	:=	0
export VERSION	:=	$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)

export UL_DEFS	:=	-DUL_MAJOR=$(VERSION_MAJOR) -DUL_MINOR=$(VERSION_MINOR) -DUL_MICRO=$(VERSION_MICRO) -DUL_VERSION=\"$(VERSION)\"

.PHONY: all fresh clean pu arc usystem uloader umenu umanager uscreen udesigner

all: arc usystem uloader umenu umanager uscreen udesigner

fresh: clean all

pu:
	@$(MAKE) -C libs/Plutonium/

arc:
	@python arc/arc.py gen_db default+./libs/uCommon/include/ul/ul_Results.rc.hpp
	@python arc/arc.py gen_cpp rc UL ./libs/uCommon/include/ul/ul_Results.gen.hpp

clean:
	@$(MAKE) clean -C projects/uSystem
	@$(MAKE) clean -C projects/uLoader
	@$(MAKE) clean -C projects/uMenu
	@$(MAKE) clean -C projects/uManager
	@$(MAKE) clean -C projects/uDesigner
	@cd projects/uScreen && mvn clean
	@rm -rf SdOut

usystem: arc
	@$(MAKE) -C projects/uSystem
	@mkdir -p SdOut/atmosphere/contents/0100000000001000
	@cp projects/uSystem/out/nintendo_nx_arm64_armv8a/release/uSystem.nsp SdOut/atmosphere/contents/0100000000001000/exefs.nsp
	@mkdir -p SdOut/ulaunch/bin/uSystem
	@cp projects/uSystem/out/nintendo_nx_arm64_armv8a/release/uSystem.nsp SdOut/ulaunch/bin/uSystem/exefs.nsp

uloader: arc
	@$(MAKE) -C projects/uLoader
	@mkdir -p SdOut/ulaunch/bin/uLoader
	@mkdir -p SdOut/ulaunch/bin/uLoader/applet
	@cp projects/uLoader/uLoader.nso SdOut/ulaunch/bin/uLoader/applet/main
	@cp projects/uLoader/uLoader_applet.npdm SdOut/ulaunch/bin/uLoader/applet/main.npdm
	@mkdir -p SdOut/ulaunch/bin/uLoader/application
	@cp projects/uLoader/uLoader.nso SdOut/ulaunch/bin/uLoader/application/main
	@cp projects/uLoader/uLoader_application.npdm SdOut/ulaunch/bin/uLoader/application/main.npdm

umenu: arc pu
	@$(MAKE) -C projects/uMenu
	@mkdir -p SdOut/ulaunch/bin/uMenu
	@mkdir -p SdOut/ulaunch/lang/uMenu
	@mkdir -p SdOut/ulaunch/themes
	@cp projects/uMenu/uMenu.nso SdOut/ulaunch/bin/uMenu/main
	@cp projects/uMenu/uMenu.npdm SdOut/ulaunch/bin/uMenu/main.npdm
	@cp assets/Logo.png projects/uMenu/romfs/Logo.png
	@cp -r default-theme/ projects/uMenu/romfs/default/
	@cp projects/uMenu/romfs/Logo.png projects/uDesigner/assets/Logo.png
	@build_romfs projects/uMenu/romfs SdOut/ulaunch/bin/uMenu/romfs.bin

umanager: arc pu
	@$(MAKE) -C libs/Plutonium/

arc:
	@python arc/arc.py gen_db default+./libs/uCommon/include/ul/ul_Results.rc.hpp
	@python arc/arc.py gen_cpp rc UL ./libs/uCommon/include/ul/ul_Results.gen.hpp

clean:
	@$(MAKE) clean -C projects/uSystem
	@$(MAKE) clean -C projects/uLoader
	@$(MAKE) clean -C projects/uMenu
	@$(MAKE) clean -C projects/uManager
	@$(MAKE) clean -C projects/uDesigner
	@cd projects/uScreen && mvn clean
	@rm -rf SdOut

usystem: arc
	@$(MAKE) -C projects/uSystem
	@mkdir -p SdOut/atmosphere/contents/0100000000001000
	@cp projects/uSystem/out/nintendo_nx_arm64_armv8a/release/uSystem.nsp SdOut/atmosphere/contents/0100000000001000/exefs.nsp
	@mkdir -p SdOut/ulaunch/bin/uSystem
	@cp projects/uSystem/out/nintendo_nx_arm64_armv8a/release/uSystem.nsp SdOut/ulaunch/bin/uSystem/exefs.nsp

uloader: arc
	@$(MAKE) -C projects/uLoader
	@mkdir -p SdOut/ulaunch/bin/uLoader
	@mkdir -p SdOut/ulaunch/bin/uLoader/applet
	@cp projects/uLoader/uLoader.nso SdOut/ulaunch/bin/uLoader/applet/main
	@cp projects/uLoader/uLoader_applet.npdm SdOut/ulaunch/bin/uLoader/applet/main.npdm
	@mkdir -p SdOut/ulaunch/bin/uLoader/application
	@cp projects/uLoader/uLoader.nso SdOut/ulaunch/bin/uLoader/application/main
	@cp projects/uLoader/uLoader_application.npdm SdOut/ulaunch/bin/uLoader/application/main.npdm

umenu: arc pu
	@$(MAKE) -C projects/uMenu
	@mkdir -p SdOut/ulaunch/bin/uMenu
	@mkdir -p SdOut/ulaunch/lang/uMenu
	@mkdir -p SdOut/ulaunch/themes
	@cp projects/uMenu/uMenu.nso SdOut/ulaunch/bin/uMenu/main
	@cp projects/uMenu/uMenu.npdm SdOut/ulaunch/bin/uMenu/main.npdm
	@cp assets/Logo.png projects/uMenu/romfs/Logo.png
	@cp -r default-theme/ projects/uMenu/romfs/default/
	@cp projects/uMenu/romfs/Logo.png projects/uDesigner/assets/Logo.png
	@build_romfs projects/uMenu/romfs SdOut/ulaunch/bin/uMenu/romfs.bin

umanager: arc pu
	@$(MAKE) -C projects/uManager
	@mkdir -p SdOut/ulaunch/lang/uManager
	@mkdir -p SdOut/switch
	@cp projects/uManager/uManager.nro SdOut/switch/uManager.nro

uscreen:
	@cd projects/uScreen && mvn package

udesigner: umenu
	@cd default-theme && zip -r default-theme.zip theme ui sound
	@mv default-theme/default-theme.zip projects/uDesigner/assets/default-theme.zip
	@cp projects/uMenu/romfs/Logo.png projects/uDesigner/assets/Logo.png
	@$(MAKE) -C projects/uDesigner
