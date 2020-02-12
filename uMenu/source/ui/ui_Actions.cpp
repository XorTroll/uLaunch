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

extern ui::MenuApplication::Ref g_menu_app_instance;
extern cfg::Config g_ul_config;

namespace ui::actions
{
    void ShowAboutDialog()
    {
        g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "ulaunch_about"), "uLaunch v" + std::string(UL_VERSION) + "\n\n" + cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "ulaunch_desc") + ":\nhttps://github.com/XorTroll/uLaunch", { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "ok") }, true, "romfs:/LogoLarge.png");
    }

    void ShowSettingsMenu()
    {
        g_menu_app_instance->FadeOut();
        g_menu_app_instance->LoadSettingsMenu();
        g_menu_app_instance->FadeIn();
    }

    void ShowThemesMenu()
    {
        g_menu_app_instance->FadeOut();
        g_menu_app_instance->LoadThemeMenu();
        g_menu_app_instance->FadeIn();
    }

    void ShowUserMenu()
    {
        auto uid = g_menu_app_instance->GetSelectedUser();

        auto [_rc, name] = os::GetAccountName(uid);
        auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "user_settings"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "user_selected") + ": " + name + "\n" + cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "user_option"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "user_view_page"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "user_logoff"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "cancel") }, true, os::GetIconCacheImagePath(uid));
        if(sopt == 0) friendsLaShowMyProfileForHomeMenu(uid);
        else if(sopt == 1)
        {
            u32 logoff = 0;
            if(g_menu_app_instance->IsSuspended())
            {
                auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "suspended_app"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "user_logoff_app_suspended"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "yes"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "cancel") }, true);
                if(sopt == 0) logoff = 2;
            }
            else logoff = 1;

            if(logoff > 0)
            {
                auto &menu_lyt = g_menu_app_instance->GetMenuLayout();
                if(logoff == 2) menu_lyt->DoTerminateApplication();
                g_menu_app_instance->FadeOut();
                menu_lyt->MoveFolder("", false);
                g_menu_app_instance->LoadStartupMenu();
                g_menu_app_instance->FadeIn();
            }
        }
    }

    void ShowControllerSupport()
    {
        HidLaControllerSupportArg arg = {};
        hidLaCreateControllerSupportArg(&arg);
        arg.enable_explain_text = true;
        for(u32 i = 0; i < 8; i++) strcpy(arg.explain_text[i], "Test explain text");
        hidLaShowControllerSupportForSystem(nullptr, &arg, true);
    }

    void ShowWebPage()
    {
        SwkbdConfig swkbd;
        swkbdCreate(&swkbd, 0);
        swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "swkbd_webpage_guide").c_str());
        char url[500] = {0};
        auto rc = swkbdShow(&swkbd, url, 500);
        swkbdClose(&swkbd);
        if(R_SUCCEEDED(rc))
        {
            WebCommonConfig web = {};
            webPageCreate(&web, url);
            webConfigSetWhitelist(&web, ".*");

            am::MenuCommandWriter writer(am::DaemonMessage::OpenWebPage);
            writer.Write<WebCommonConfig>(web);
            writer.FinishWrite();

            g_menu_app_instance->StopPlayBGM();
            g_menu_app_instance->CloseWithFadeOut();
        }
    }

    void ShowHelpDialog()
    {
        std::string msg;
        msg += " - " + cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "help_launch") + "\n";
        msg += " - " + cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "help_close") + "\n";
        msg += " - " + cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "help_quick") + "\n";
        msg += " - " + cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "help_multiselect") + "\n";
        msg += " - " + cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "help_back") + "\n";
        msg += " - " + cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "help_minus") + "\n";
        msg += " - " + cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "help_plus") + "\n";

        g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "help_title"), msg, { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "ok") }, true);
    }

    void ShowAlbumApplet()
    {
        am::MenuCommandWriter writer(am::DaemonMessage::OpenAlbum);
        writer.FinishWrite();

        g_menu_app_instance->StopPlayBGM();
        g_menu_app_instance->CloseWithFadeOut();
    }

    void ShowPowerDialog()
    {
        auto msg = os::GeneralChannelMessage::Invalid;

        auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "power_dialog"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "power_dialog_info"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "power_sleep"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "power_power_off"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "power_reboot"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "cancel") }, true);
        if(sopt == 0) msg = os::GeneralChannelMessage::Sleep;
        else if(sopt == 1) msg = os::GeneralChannelMessage::Shutdown;
        else if(sopt == 2) msg = os::GeneralChannelMessage::Reboot;

        if(msg != os::GeneralChannelMessage::Invalid)
        {
            // Fade out on all cases
            g_menu_app_instance->FadeOut();
            os::SystemAppletMessage smsg = {};
            smsg.magic = os::SAMSMagic;
            smsg.message = (u32)msg;

            os::PushSystemAppletMessage(smsg);
            svcSleepThread(1'500'000'000L);

            // When we get back after sleep we will do a cool fade in, whereas wuth the other options the console will be already off/rebooted
            g_menu_app_instance->FadeIn();
        }
    }

}