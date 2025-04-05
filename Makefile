VERSION_MAJOR	:=	1
VERSION_MINOR	:=	2
VERSION_MICRO	:=	0
export VERSION	:=	$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)

export UL_DEFS	:=	-DUL_MAJOR=$(VERSION_MAJOR) -DUL_MINOR=$(VERSION_MINOR) -DUL_MICRO=$(VERSION_MICRO) -DUL_VERSION=\"$(VERSION)\"

OUT_DIR				:=	SdOut
OUT_DIR_ZIP			:=	uLaunch-v$(VERSION)

OUT_THEME_MUSIC	:=	default-theme-music-v$(VERSION).ultheme
THEME_MUSIC_TEMP	:=	default-theme-music-tmp

.PHONY: all fresh clean pu arc usystem uloader umenu umanager uscreen udesigner default-theme-music package

all: arc usystem uloader umenu umanager uscreen udesigner default-theme-music package

fresh: clean all

clean:
	@$(MAKE) clean -C projects/uSystem
	@$(MAKE) clean -C projects/uLoader
	@$(MAKE) clean -C projects/uMenu
	@$(MAKE) clean -C projects/uManager
	@$(MAKE) clean -C projects/uDesigner
	@cd projects/uScreen && mvn clean
	@rm -rf $(OUT_DIR)

pu:
	@$(MAKE) -C libs/Plutonium/

arc:
	@python arc/arc.py gen_db default+./libs/uCommon/include/ul/ul_Results.rc.hpp
	@python arc/arc.py gen_cpp rc UL ./libs/uCommon/include/ul/ul_Results.gen.hpp

usystem: arc
	@$(MAKE) -C projects/uSystem
	@mkdir -p $(OUT_DIR)/atmosphere/contents/0100000000001000
	@cp projects/uSystem/out/nintendo_nx_arm64_armv8a/release/uSystem.nsp $(OUT_DIR)/atmosphere/contents/0100000000001000/exefs.nsp
	@mkdir -p $(OUT_DIR)/ulaunch/bin/uSystem
	@cp projects/uSystem/out/nintendo_nx_arm64_armv8a/release/uSystem.nsp $(OUT_DIR)/ulaunch/bin/uSystem/exefs.nsp

uloader: arc
	@$(MAKE) -C projects/uLoader
	@mkdir -p $(OUT_DIR)/ulaunch/bin/uLoader
	@mkdir -p $(OUT_DIR)/ulaunch/bin/uLoader/applet
	@cp projects/uLoader/uLoader.nso $(OUT_DIR)/ulaunch/bin/uLoader/applet/main
	@cp projects/uLoader/uLoader_applet.npdm $(OUT_DIR)/ulaunch/bin/uLoader/applet/main.npdm
	@mkdir -p $(OUT_DIR)/ulaunch/bin/uLoader/application
	@cp projects/uLoader/uLoader.nso $(OUT_DIR)/ulaunch/bin/uLoader/application/main
	@cp projects/uLoader/uLoader_application.npdm $(OUT_DIR)/ulaunch/bin/uLoader/application/main.npdm

umenu: arc pu
	@$(MAKE) -C projects/uMenu
	@mkdir -p $(OUT_DIR)/ulaunch/bin/uMenu
	@mkdir -p $(OUT_DIR)/ulaunch/lang/uMenu
	@mkdir -p $(OUT_DIR)/ulaunch/themes
	@cp projects/uMenu/uMenu.nso $(OUT_DIR)/ulaunch/bin/uMenu/main
	@cp projects/uMenu/uMenu.npdm $(OUT_DIR)/ulaunch/bin/uMenu/main.npdm
	@cp assets/Logo.png projects/uMenu/romfs/Logo.png
	@rm -rf projects/uMenu/romfs/default
	@cp -r default-theme/ projects/uMenu/romfs/default/
	@build_romfs projects/uMenu/romfs $(OUT_DIR)/ulaunch/bin/uMenu/romfs.bin

umanager: arc pu
	@$(MAKE) -C projects/uManager
	@mkdir -p $(OUT_DIR)/ulaunch/lang/uManager
	@mkdir -p $(OUT_DIR)/switch
	@cp projects/uManager/uManager.nro $(OUT_DIR)/switch/uManager.nro

uscreen:
	@cd projects/uScreen && mvn package

udesigner:
	@cd default-theme && rm -rf default-theme.ultheme && zip -r default-theme.ultheme theme ui sound
	@mv default-theme/default-theme.ultheme projects/uDesigner/assets/default-theme.ultheme
	@cp assets/Logo.png projects/uDesigner/assets/Logo.png
	@$(MAKE) -C projects/uDesigner

default-theme-music:
	@echo "Creating default-theme-music..."
	@rm -rf $(THEME_MUSIC_TEMP)
	@rm -rf $(OUT_THEME_MUSIC)
	@mkdir -p $(THEME_MUSIC_TEMP)
	@cp -r default-theme/ui $(THEME_MUSIC_TEMP)/ui
	@cp -r default-theme-music/theme $(THEME_MUSIC_TEMP)/theme
	@cp -r default-theme-music/sound $(THEME_MUSIC_TEMP)/sound
	@cd $(THEME_MUSIC_TEMP) && zip -r ../$(OUT_THEME_MUSIC) sound theme ui
	@rm -rf $(THEME_MUSIC_TEMP)
	@echo "Created default-theme-music: $(OUT_THEME_MUSIC)!"

package:
	@rm -rf $(OUT_DIR_ZIP).7z $(OUT_DIR_ZIP).zip
	@cd $(OUT_DIR) && 7z a ../$(OUT_DIR_ZIP).7z atmosphere ulaunch switch
	@cd $(OUT_DIR) && zip -r ../$(OUT_DIR_ZIP).zip atmosphere ulaunch switch
	@echo "Packaged $(OUT_DIR) into $(OUT_DIR_ZIP).7z & $(OUT_DIR_ZIP).zip!"
