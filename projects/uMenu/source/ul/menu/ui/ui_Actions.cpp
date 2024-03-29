#include <ul/menu/ui/ui_Actions.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/menu/ui/ui_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/system/system_Message.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/acc/acc_Accounts.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::menu::ui::TransitionGuard g_TransitionGuard;
extern ul::cfg::Config g_Config;

namespace ul::menu::ui {

    namespace {

        inline void PushPowerSystemAppletMessage(const system::GeneralChannelMessage msg) {
            // Fade out on all cases
            g_MenuApplication->FadeOut();

            system::PushSimpleSystemAppletMessage(msg);
            svcSleepThread(1'500'000'000ul);

            // When we get back after sleep we will do a cool fade in, whereas with the other options the console will be already off/rebooted
            g_MenuApplication->FadeIn();
        }

        const std::vector<std::string> g_ImageFormatList = { "png", "jpg", "jpeg", "webp" };

    }

    std::string TryFindImage(const cfg::Theme &theme, const std::string &path_no_ext) {
        for(const auto &fmt: g_ImageFormatList) {
            const auto path = cfg::GetAssetByTheme(theme, path_no_ext + "." + fmt);
            if(!path.empty()) {
                return path;
            }
        }

        return "";
    }
    
    pu::sdl2::Texture TryFindLoadImage(const cfg::Theme &theme, const std::string &path_no_ext) {
        for(const auto &fmt: g_ImageFormatList) {
            const auto path = cfg::GetAssetByTheme(theme, path_no_ext + "." + fmt);
            const auto img = pu::ui::render::LoadImage(path);
            if(img != nullptr) {
                return img;
            }
        }

        return nullptr;
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
        g_MenuApplication->CreateShowDialog(GetLanguageString("ulaunch_about"), "uLaunch v" + std::string(UL_VERSION) + "\n\n" + GetLanguageString("ulaunch_desc") + ":\nhttps://github.com/XorTroll/uLaunch", { GetLanguageString("ok") }, true, "romfs:/LogoLarge.png");
    }

    void ShowSettingsMenu() {
        g_TransitionGuard.Run([]() {
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadSettingsMenu();
            g_MenuApplication->FadeIn();
        });
    }

    void ShowThemesMenu() {
        g_TransitionGuard.Run([]() {
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadThemeMenu();
            g_MenuApplication->FadeIn();
        });
    }

    void ShowUserMenu() {
        const auto uid = g_MenuApplication->GetSelectedUser();
        std::string name;
        acc::GetAccountName(uid, name);
        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("user_settings"), GetLanguageString("user_selected") + ": " + name + "\n" + GetLanguageString("user_option"), { GetLanguageString("user_view_page"), GetLanguageString("user_logoff"), GetLanguageString("cancel") }, true, acc::GetIconCacheImagePath(uid) );
        if(option == 0) {
            friendsLaShowMyProfileForHomeMenu(uid);
        }
        else if(option == 1) {
            auto log_off = false;
            if(g_MenuApplication->IsSuspended()) {
                const auto option_2 = g_MenuApplication->CreateShowDialog(GetLanguageString("suspended_app"), GetLanguageString("user_logoff_app_suspended"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                if(option_2 == 0) {
                    log_off = true;
                }
            }
            else {
                log_off = true;
            }

            if(log_off) {
                auto &main_menu_lyt = g_MenuApplication->GetMainMenuLayout();
                if(g_MenuApplication->IsSuspended()) {
                    main_menu_lyt->DoTerminateApplication();
                }

                g_TransitionGuard.Run([&]() {
                    g_MenuApplication->FadeOut();
                    main_menu_lyt->MoveToRoot(false);
                    g_MenuApplication->LoadStartupMenu();
                    g_MenuApplication->FadeIn();
                });
            }
        }
    }

    void ShowControllerSupport() {
        HidLaControllerSupportArg arg = {};
        hidLaCreateControllerSupportArg(&arg);
        arg.enable_explain_text = true;
        for(u32 i = 0; i < 8; i++) {
            util::CopyToStringBuffer(arg.explain_text[i], GetLanguageString("controller_support_explain_text"));
        }
        hidLaShowControllerSupportForSystem(nullptr, &arg, true);
    }

    void ShowWebPage() {
        SwkbdConfig swkbd;
        if(R_SUCCEEDED(swkbdCreate(&swkbd, 0))) {
            UL_ON_SCOPE_EXIT({
                swkbdClose(&swkbd);
            });
            
            swkbdConfigSetGuideText(&swkbd, GetLanguageString("swkbd_webpage_guide").c_str());
            
            char url[500] = {};
            if(R_SUCCEEDED(swkbdShow(&swkbd, url, sizeof(url)))) {
                UL_RC_ASSERT(ul::menu::smi::OpenWebPage(url));

                g_MenuApplication->StopPlayBGM();
                g_MenuApplication->CloseWithFadeOut();
            }
        }
    }

    void ShowAlbumApplet() {
        // TODONEW: somehow force to load actual album?
        UL_RC_ASSERT(ul::menu::smi::OpenAlbum());

        g_MenuApplication->StopPlayBGM();
        g_MenuApplication->CloseWithFadeOut();
    }

    void ShowPowerDialog() {
        auto msg = ul::system::GeneralChannelMessage::Unk_Invalid;

        auto sopt = g_MenuApplication->CreateShowDialog(GetLanguageString("power_dialog"), GetLanguageString("power_dialog_info"), { GetLanguageString("power_sleep"), GetLanguageString("power_power_off"), GetLanguageString("power_reboot"), GetLanguageString("cancel") }, true);
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