
#pragma once
#include <ul/cfg/cfg_Config.hpp>
#include <pu/Plutonium>

namespace ul::menu::ui {

    std::string TryFindImage(const cfg::Theme &theme, const std::string &path_no_ext);
    pu::sdl2::Texture TryFindLoadImage(const cfg::Theme &theme, const std::string &path_no_ext);

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