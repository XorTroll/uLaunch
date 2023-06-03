#include <ul/menu/ui/ui_Util.hpp>
#include <ul/menu/ui/ui_Application.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/system/system_Message.hpp>
#include <ul/util/util_Scope.hpp>

extern ul::menu::ui::Application::Ref g_Application;
// extern ui::TransitionGuard g_TransitionGuard;
// extern cfg::Config g_Config;
extern AccountUid g_SelectedUser;

namespace ul::menu::ui {

    namespace {

        void HandleSystemGeneralChannelMessage(const system::GeneralChannelMessage msg) {
            // Fade out on all cases
            g_Application->FadeOut();

            UL_RC_ASSERT(system::PushSimpleSystemAppletMessage(msg));
            svcSleepThread(1'500'000'000ul);

            // When we get back after sleep we will do a cool fade in, whereas with the other options the console will be already off/rebooted
            g_Application->FadeIn();
        }

    }

    void ShowAboutDialog() {
        g_Application->CreateShowDialog("About", "uMad " UL_VERSION " dev build!", { "Ok" }, true);
        // g_MenuApplication->CreateShowDialog(GetLanguageString("ulaunch_about"), "uLaunch v" + std::string(UL_VERSION) + "\n\n" + GetLanguageString("ulaunch_desc") + ":\nhttps://github.com/XorTroll/uLaunch", { GetLanguageString("ok") }, true, "romfs:/LogoLarge.png");
    }

    void ShowMainMenu() {
        g_Application->FadeOut();
        g_Application->LoadMainMenu();
        g_Application->FadeIn();
    }
    
    void ShowSettingsMenu() {
        g_Application->FadeOut();
        g_Application->LoadSettingsMenu();
        g_Application->FadeIn();
    }

    void ShowThemesMenu() {
        g_Application->CreateShowDialog("Themes", "Still not implemented...", { "Ok" }, true);
        /*
        g_TransitionGuard.Run([]() {
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadThemeMenu();
            g_MenuApplication->FadeIn();
        });
        */
    }

    void ShowUserMenu() {
        friendsLaShowMyProfileForHomeMenu(g_SelectedUser);
        /*
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
        */
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
        UL_RC_ASSERT(swkbdCreate(&swkbd, 0));
        util::OnScopeExit swkbd_close([&]() {
            swkbdClose(&swkbd);
        });
        
        swkbdConfigSetGuideText(&swkbd, "Type webpage");
        
        char url[500] = {0};
        const auto rc = swkbdShow(&swkbd, url, 500);
        swkbdClose(&swkbd);

        if(R_SUCCEEDED(rc)) {
            UL_RC_ASSERT(smi::OpenWebPage(url));

            // g_Application->StopPlayBGM();
            g_Application->CloseWithFadeOut();
        }
    }

    void ShowMiiEdit() {
        UL_RC_ASSERT(miiLaShowMiiEdit(MiiSpecialKeyCode_Normal));
    }

    void ShowAmiiboSettings() {
        const auto option = g_Application->CreateShowDialog("Amiibo settings", "Select option", { "Change nickname/owner", "Remove game info", "Format", "Cancel" }, true);
        if(option == 0) {
            
        }
        else if(option == 1) {
               
        }
        else if(option == 2) {

        }
    }

    void ShowHelpDialog() {
        g_Application->CreateShowDialog("Help", "Dummy help dialog", { "Ok" }, true);
        /*
        std::string msg;
        msg += " - " + GetLanguageString("help_launch") + "\n";
        msg += " - " + GetLanguageString("help_close") + "\n";
        msg += " - " + GetLanguageString("help_quick") + "\n";
        msg += " - " + GetLanguageString("help_multiselect") + "\n";
        msg += " - " + GetLanguageString("help_back") + "\n";
        msg += " - " + GetLanguageString("help_minus") + "\n";
        msg += " - " + GetLanguageString("help_plus") + "\n";

        g_MenuApplication->CreateShowDialog(GetLanguageString("help_title"), msg, { GetLanguageString("ok") }, true);
        */
    }

    void ShowAlbumApplet() {
        UL_RC_ASSERT(smi::OpenAlbum());

        // g_Application->StopPlayBGM();
        g_Application->CloseWithFadeOut();
    }

    void Sleep() {
        HandleSystemGeneralChannelMessage(system::GeneralChannelMessage::Unk_Sleep);
    }

    void PowerOff() {
        HandleSystemGeneralChannelMessage(system::GeneralChannelMessage::Unk_Shutdown);
    }

    void Reboot() {
        HandleSystemGeneralChannelMessage(system::GeneralChannelMessage::Unk_Reboot);
    }

    void ShowPowerDialog() {
        auto msg = system::GeneralChannelMessage::Unk_Invalid;

        const auto opt = g_Application->CreateShowDialog("Power dialog", "Power dialog info", { "Sleep", "Power off", "Reboot", "Cancel" }, true);
        if(opt == 0) {
            msg = system::GeneralChannelMessage::Unk_Sleep;
        }
        else if(opt == 1) {
            msg = system::GeneralChannelMessage::Unk_Shutdown;
        }
        else if(opt == 2) {
            msg = system::GeneralChannelMessage::Unk_Reboot;
        }

        if(msg != system::GeneralChannelMessage::Unk_Invalid) {
            HandleSystemGeneralChannelMessage(msg);
        }
    }

}