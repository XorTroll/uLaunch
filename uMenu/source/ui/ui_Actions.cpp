#include <ui/ui_Actions.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <ui/ui_MenuLayout.hpp>
#include <os/os_Titles.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <util/util_Misc.hpp>
#include <os/os_Misc.hpp>
#include <am/am_LibraryApplet.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <os/os_HomeMenu.hpp>
#include <fs/fs_Stdio.hpp>
#include <net/net_Service.hpp>

extern ui::MenuApplication::Ref g_MenuApplication;
extern cfg::Config g_Config;

namespace ui::actions {

    void ShowAboutDialog() {
        g_MenuApplication->CreateShowDialog(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "ulaunch_about"), "uLaunch v" + std::string(UL_VERSION) + "\n\n" + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "ulaunch_desc") + ":\nhttps://github.com/XorTroll/uLaunch", { cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "ok") }, true, "romfs:/LogoLarge.png");
    }

    void ShowSettingsMenu() {
        g_MenuApplication->FadeOut();
        g_MenuApplication->LoadSettingsMenu();
        g_MenuApplication->FadeIn();
    }

    void ShowThemesMenu() {
        g_MenuApplication->FadeOut();
        g_MenuApplication->LoadThemeMenu();
        g_MenuApplication->FadeIn();
    }

    void ShowUserMenu() {
        auto uid = g_MenuApplication->GetSelectedUser();
        std::string name;
        os::GetAccountName(name, uid);
        auto sopt = g_MenuApplication->CreateShowDialog(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "user_settings"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "user_selected") + ": " + name + "\n" + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "user_option"), { cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "user_view_page"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "user_logoff"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "cancel") }, true, os::GetIconCacheImagePath(uid));
        if(sopt == 0) {
            friendsLaShowMyProfileForHomeMenu(uid);
        }
        else if(sopt == 1) {
            u32 logoff = 0;
            if(g_MenuApplication->IsSuspended()) {
                auto sopt = g_MenuApplication->CreateShowDialog(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "suspended_app"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "user_logoff_app_suspended"), { cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "yes"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "cancel") }, true);
                if(sopt == 0) {
                    logoff = 2;
                }
            }
            else {
                logoff = 1;
            }
            if(logoff > 0) {
                auto &menu_lyt = g_MenuApplication->GetMenuLayout();
                if(logoff == 2) {
                    menu_lyt->DoTerminateApplication();
                }
                g_MenuApplication->FadeOut();
                menu_lyt->MoveFolder("", false);
                g_MenuApplication->LoadStartupMenu();
                g_MenuApplication->FadeIn();
            }
        }
    }

    void ShowControllerSupport() {
        HidLaControllerSupportArg arg = {};
        hidLaCreateControllerSupportArg(&arg);
        arg.enable_explain_text = true;
        for(u32 i = 0; i < 8; i++) {
            strcpy(arg.explain_text[i], "Test explain text");
        }
        hidLaShowControllerSupportForSystem(nullptr, &arg, true);
    }

    void ShowWebPage() {
        SwkbdConfig swkbd;
        swkbdCreate(&swkbd, 0);
        UL_ON_SCOPE_EXIT({
            swkbdClose(&swkbd);
        });
        
        swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "swkbd_webpage_guide").c_str());
        
        char url[500] = {0};
        swkbdShow(&swkbd, url, 500);

        UL_ASSERT(dmi::menu::SendCommand(dmi::DaemonMessage::OpenWebPage, [&](dmi::menu::MenuScopedStorageWriter &writer) {
            R_TRY(writer.PushData(url, sizeof(url)));
            return ResultSuccess;
        },
        [&](dmi::menu::MenuScopedStorageReader &reader) {
            // ...
            return ResultSuccess;
        }));

        g_MenuApplication->StopPlayBGM();
        g_MenuApplication->CloseWithFadeOut();
    }

    void ShowHelpDialog() {
        std::string msg;
        msg += " - " + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "help_launch") + "\n";
        msg += " - " + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "help_close") + "\n";
        msg += " - " + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "help_quick") + "\n";
        msg += " - " + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "help_multiselect") + "\n";
        msg += " - " + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "help_back") + "\n";
        msg += " - " + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "help_minus") + "\n";
        msg += " - " + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "help_plus") + "\n";

        g_MenuApplication->CreateShowDialog(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "help_title"), msg, { cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "ok") }, true);
    }

    void ShowAlbumApplet() {
        UL_ASSERT(dmi::menu::SendCommand(dmi::DaemonMessage::OpenAlbum, [&](dmi::menu::MenuScopedStorageWriter &writer) {
            // ...
            return ResultSuccess;
        },
        [&](dmi::menu::MenuScopedStorageReader &reader) {
            // ...
            return ResultSuccess;
        }));

        g_MenuApplication->StopPlayBGM();
        g_MenuApplication->CloseWithFadeOut();
    }

    void ShowPowerDialog() {
        auto msg = os::GeneralChannelMessage::Invalid;

        auto sopt = g_MenuApplication->CreateShowDialog(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "power_dialog"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "power_dialog_info"), { cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "power_sleep"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "power_power_off"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "power_reboot"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "cancel") }, true);
        if(sopt == 0) {
            msg = os::GeneralChannelMessage::Sleep;
        }
        else if(sopt == 1) {
            msg = os::GeneralChannelMessage::Shutdown;
        }
        else if(sopt == 2) {
            msg = os::GeneralChannelMessage::Reboot;
        }

        if(msg != os::GeneralChannelMessage::Invalid) {
            // Fade out on all cases
            g_MenuApplication->FadeOut();
            
            auto smsg = os::SystemAppletMessage::Create(msg);
            os::PushSystemAppletMessage(smsg);
            svcSleepThread(1'500'000'000ul);

            // When we get back after sleep we will do a cool fade in, whereas with the other options the console will be already off/rebooted
            g_MenuApplication->FadeIn();
        }
    }

}