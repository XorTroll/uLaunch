#include <ul/menu/ui/ui_Common.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/menu/ui/ui_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/system/system_Message.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/acc/acc_Accounts.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::cfg::Config g_Config;
extern ul::cfg::Theme g_ActiveTheme;

namespace ul::menu::ui {

    namespace {

        inline void PushPowerSystemAppletMessage(const system::GeneralChannelMessage msg) {
            // Fade out (black) on all cases
            g_MenuApplication->ResetFade();
            g_MenuApplication->FadeOut();

            system::PushSimpleSystemAppletMessage(msg);
            svcSleepThread(1'500'000'000ul);

            // When we get back after sleep we will do a cool fade in, whereas with the other options the console will be already off/rebooted
            g_MenuApplication->FadeIn();
        }

        bool g_CommonTexturesLoaded = false;
        pu::sdl2::TextureHandle::Ref g_BackgroundTexture;
        pu::sdl2::TextureHandle::Ref g_LogoTexture;

        pu::sdl2::TextureHandle::Ref g_NonEditableSettingIconTexture;
        pu::sdl2::TextureHandle::Ref g_EditableSettingIconTexture;

        pu::sdl2::TextureHandle::Ref g_UserIconTexture;

    }

    std::string TryGetActiveThemeResource(const std::string &resource_base) {
        std::string path;
        if(g_ActiveTheme.IsValid()) {
            path = cfg::GetActiveThemeResource(resource_base);
        }
        else {
            path = fs::JoinPath(DefaultThemePath, resource_base);
        }

        if(!fs::ExistsFile(path)) {
            path = "";
        }
        return path;
    }

    std::string TryFindImage(const std::string &path_no_ext) {
        for(const auto &fmt: ImageFormatList) {
            const auto path = TryGetActiveThemeResource(path_no_ext + "." + fmt);
            if(!path.empty()) {
                return path;
            }
        }

        return "";
    }
    
    pu::sdl2::Texture TryFindLoadImage(const std::string &path_no_ext) {
        for(const auto &fmt: ImageFormatList) {
            const auto path = TryGetActiveThemeResource(path_no_ext + "." + fmt);
            const auto img = pu::ui::render::LoadImage(path);
            if(img != nullptr) {
                return img;
            }
        }

        return nullptr;
    }

    void LoadCommonTextures() {
        if(!g_CommonTexturesLoaded) {
            g_BackgroundTexture = TryFindLoadImageHandle("ui/Background");
            g_LogoTexture = pu::sdl2::TextureHandle::New(pu::ui::render::LoadImage("romfs:/Logo.png"));

            g_NonEditableSettingIconTexture = TryFindLoadImageHandle("ui/Settings/SettingNonEditableIcon");
            g_EditableSettingIconTexture = TryFindLoadImageHandle("ui/Settings/SettingEditableIcon");

            g_CommonTexturesLoaded = true;
        }
    }

    pu::sdl2::TextureHandle::Ref GetBackgroundTexture() {
        return g_BackgroundTexture;
    }

    pu::sdl2::TextureHandle::Ref GetLogoTexture() {
        return g_LogoTexture;
    }

    pu::sdl2::TextureHandle::Ref GetEditableSettingIconTexture() {
        return g_EditableSettingIconTexture;
    }

    pu::sdl2::TextureHandle::Ref GetNonEditableSettingIconTexture() {
        return g_NonEditableSettingIconTexture;
    }

    void LoadSelectedUserIconTexture() {
        const auto icon_path = acc::GetIconCacheImagePath(g_MenuApplication->GetSelectedUser());
        g_UserIconTexture = pu::sdl2::TextureHandle::New(pu::ui::render::LoadImage(icon_path));
    }

    pu::sdl2::TextureHandle::Ref GetSelectedUserIconTexture() {
        return g_UserIconTexture;
    }

    void SaveConfig() {
        cfg::SaveConfig(g_Config);
        UL_RC_ASSERT(smi::ReloadConfig());
    }

    void RebootSystem() {
        PushPowerSystemAppletMessage(system::GeneralChannelMessage::Unk_Reboot);
    }

    void ShutdownSystem() {
        PushPowerSystemAppletMessage(system::GeneralChannelMessage::Unk_Shutdown);
    }

    void SleepSystem() {
        PushPowerSystemAppletMessage(system::GeneralChannelMessage::Unk_Sleep);
    }

    void ShowAboutDialog() {
        g_MenuApplication->DisplayDialog("uLaunch v" + std::string(UL_VERSION), GetLanguageString("ulaunch_about") + ":\nhttps://github.com/XorTroll/uLaunch", { GetLanguageString("ok") }, true, g_LogoTexture);
    }

    void ShowSettingsMenu() {
        g_MenuApplication->LoadMenuByType(MenuType::Settings);
    }

    void ShowThemesMenu() {
        g_MenuApplication->LoadMenuByType(MenuType::Themes);
    }

    void ShowUserPage() {
        UL_RC_ASSERT(ul::menu::smi::OpenUserPage());

        g_MenuApplication->Finalize();
    }

    void ShowControllerSupport() {
        HidLaControllerSupportArg arg = {};
        hidLaCreateControllerSupportArg(&arg);
        hidLaShowControllerSupportForSystem(nullptr, &arg, true);
    }

    void ShowWebPage() {
        SwkbdConfig swkbd;
        if(R_SUCCEEDED(swkbdCreate(&swkbd, 0))) {
            UL_ON_SCOPE_EXIT({
                swkbdClose(&swkbd);
            });

            swkbdConfigSetInitialText(&swkbd, "https://");
            swkbdConfigSetGuideText(&swkbd, GetLanguageString("swkbd_webpage_guide").c_str());
            
            char url[500] = {};
            // TODO: check if starts with http(s), maybe even add it if user did not put it (thus links like google.com would be valid regardless)
            if(R_SUCCEEDED(swkbdShow(&swkbd, url, sizeof(url)))) {
                UL_RC_ASSERT(ul::menu::smi::OpenWebPage(url));

                g_MenuApplication->Finalize();
            }
        }
    }

    void ShowAlbum() {
        // TODO: somehow force to load actual album? maybe play with ams's keys in some way?
        UL_RC_ASSERT(ul::menu::smi::OpenAlbum());

        g_MenuApplication->Finalize();
    }

    void ShowMiiEdit() {
        UL_RC_ASSERT(ul::menu::smi::OpenMiiEdit());

        g_MenuApplication->Finalize();
    }

    void ShowNetConnect() {
        UL_RC_ASSERT(ul::menu::smi::OpenNetConnect());

        g_MenuApplication->Finalize();
    }

    void ShowPowerDialog() {
        auto msg = ul::system::GeneralChannelMessage::Unk_Invalid;

        auto sopt = g_MenuApplication->DisplayDialog(GetLanguageString("power_dialog"), GetLanguageString("power_dialog_info"), { GetLanguageString("power_sleep"), GetLanguageString("power_power_off"), GetLanguageString("power_reboot"), GetLanguageString("cancel") }, true);
        if(sopt == 0) {
            msg = ul::system::GeneralChannelMessage::Unk_Sleep;
        }
        else if(sopt == 1) {
            msg = ul::system::GeneralChannelMessage::Unk_Shutdown;
        }
        else if(sopt == 2) {
            msg = ul::system::GeneralChannelMessage::Unk_Reboot;
        }

        if(msg != ul::system::GeneralChannelMessage::Unk_Invalid) {
            PushPowerSystemAppletMessage(msg);
        }
    }

}
