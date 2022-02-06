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
extern ui::TransitionGuard g_TransitionGuard;
extern cfg::Config g_Config;

namespace ui::actions {

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
        os::GetAccountName(uid, name);
        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("user_settings"), GetLanguageString("user_selected") + ": " + name + "\n" + GetLanguageString("user_option"), { GetLanguageString("user_view_page"), GetLanguageString("user_logoff"), GetLanguageString("cancel") }, true, os::GetIconCacheImagePath(uid));
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
                auto &menu_lyt = g_MenuApplication->GetMenuLayout();
                if(g_MenuApplication->IsSuspended()) {
                    menu_lyt->DoTerminateApplication();
                }

                g_TransitionGuard.Run([&]() {
                    g_MenuApplication->FadeOut();
                    menu_lyt->MoveFolder("", false);
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
        
        swkbdConfigSetGuideText(&swkbd, GetLanguageString("swkbd_webpage_guide").c_str());
        
        char url[500] = {0};
        swkbdShow(&swkbd, url, 500);

        UL_RC_ASSERT(dmi::menu::SendCommand(dmi::DaemonMessage::OpenWebPage, [&](dmi::menu::MenuScopedStorageWriter &writer) {
            UL_RC_TRY(writer.PushData(url, sizeof(url)));
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
        msg += " - " + GetLanguageString("help_launch") + "\n";
        msg += " - " + GetLanguageString("help_close") + "\n";
        msg += " - " + GetLanguageString("help_quick") + "\n";
        msg += " - " + GetLanguageString("help_multiselect") + "\n";
        msg += " - " + GetLanguageString("help_back") + "\n";
        msg += " - " + GetLanguageString("help_minus") + "\n";
        msg += " - " + GetLanguageString("help_plus") + "\n";

        g_MenuApplication->CreateShowDialog(GetLanguageString("help_title"), msg, { GetLanguageString("ok") }, true);
    }

    void ShowAlbumApplet() {
        UL_RC_ASSERT(dmi::menu::SendCommand(dmi::DaemonMessage::OpenAlbum, [&](dmi::menu::MenuScopedStorageWriter &writer) {
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

        auto sopt = g_MenuApplication->CreateShowDialog(GetLanguageString("power_dialog"), GetLanguageString("power_dialog_info"), { GetLanguageString("power_sleep"), GetLanguageString("power_power_off"), GetLanguageString("power_reboot"), GetLanguageString("cancel") }, true);
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