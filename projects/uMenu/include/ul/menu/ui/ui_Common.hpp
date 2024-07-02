
#pragma once
#include <ul/cfg/cfg_Config.hpp>
#include <pu/Plutonium>

namespace ul::menu::ui {

    std::string TryGetActiveThemeResource(const std::string &resource_base);

    std::string TryFindImage(const std::string &path_no_ext);
    pu::sdl2::Texture TryFindLoadImage(const std::string &path_no_ext);

    inline pu::sdl2::TextureHandle::Ref TryFindLoadImageHandle(const std::string &path_no_ext) {
        return pu::sdl2::TextureHandle::New(TryFindLoadImage(path_no_ext));
    }

    void LoadCommonTextures();
    pu::sdl2::TextureHandle::Ref GetBackgroundTexture();
    pu::sdl2::TextureHandle::Ref GetLogoTexture();

    pu::sdl2::TextureHandle::Ref GetEditableSettingIconTexture();
    pu::sdl2::TextureHandle::Ref GetNonEditableSettingIconTexture();

    void LoadSelectedUserIconTexture();
    pu::sdl2::TextureHandle::Ref GetSelectedUserIconTexture();

    void SaveConfig();

    void RebootSystem();
    void ShutdownSystem();
    void SleepSystem();

    void ShowAboutDialog();
    void ShowSettingsMenu();
    void ShowThemesMenu();
    void ShowUserPage();
    void ShowControllerSupport();
    void ShowWebPage();
    void ShowAlbum();
    void ShowMiiEdit();
    void ShowPowerDialog();

}
