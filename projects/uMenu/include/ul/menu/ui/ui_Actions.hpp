
#pragma once
#include <ul/cfg/cfg_Config.hpp>

namespace ul::menu::ui {

    void RebootSystem();
    void ShutdownSystem();
    void SleepSystem();

    void ShowAboutDialog();
    void ShowSettingsMenu();
    void ShowThemesMenu();
    void ShowUserMenu();
    void ShowControllerSupport();
    void ShowWebPage();
    void ShowAlbumApplet();
    void ShowPowerDialog();

}