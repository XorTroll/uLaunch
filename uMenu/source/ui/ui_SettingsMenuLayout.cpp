#include <ui/ui_SettingsMenuLayout.hpp>
#include <os/os_Account.hpp>
#include <os/os_Misc.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <fs/fs_Stdio.hpp>
#include <net/net_Service.hpp>
#include <am/am_LibraryApplet.hpp>

extern ui::MenuApplication::Ref g_MenuApplication;
extern cfg::Theme g_Theme;
extern cfg::Config g_Config;

namespace ui {
    
    template<typename T>
    inline std::string EncodeForSettings(T t) {
        return cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_unknown_value");
    }

    template<>
    inline std::string EncodeForSettings<std::string>(std::string t) {
        return "\"" + t + "\"";
    }
    
    template<>
    inline std::string EncodeForSettings<u32>(u32 t) {
        return "\"" + std::to_string(t) + "\"";
    }

    template<>
    inline std::string EncodeForSettings<bool>(bool t) {
        return t ? cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_true_value") : cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_false_value");
    }

    SettingsMenuLayout::SettingsMenuLayout() {
        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));

        pu::ui::Color textclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        pu::ui::Color menufocusclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        pu::ui::Color menubgclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->infoText = pu::ui::elm::TextBlock::New(0, 100, cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_info_text"));
        this->infoText->SetColor(textclr);
        this->infoText->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        g_MenuApplication->ApplyConfigForElement("settings_menu", "info_text", this->infoText);
        this->Add(this->infoText);

        this->settingsMenu = pu::ui::elm::Menu::New(50, 160, 1180, menubgclr, 100, 4);
        this->settingsMenu->SetOnFocusColor(menufocusclr);
        g_MenuApplication->ApplyConfigForElement("settings_menu", "settings_menu_item", this->settingsMenu);
        this->Add(this->settingsMenu);
    }

    void SettingsMenuLayout::OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch pos) {
        if(down & KEY_B) {
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadMenu();
            g_MenuApplication->FadeIn();
        }
    }

    void SettingsMenuLayout::OnHomeButtonPress() {
        g_MenuApplication->FadeOut();
        g_MenuApplication->LoadMenu();
        g_MenuApplication->FadeIn();
    }

    void SettingsMenuLayout::Reload() {
        this->settingsMenu->ClearItems();
        this->settingsMenu->SetSelectedIndex(0);
        
        SetSysDeviceNickName console_name = {};
        setsysGetDeviceNickname(&console_name);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_console_nickname"), EncodeForSettings<std::string>(console_name.nickname), 0);
        TimeLocationName loc = {};
        timeGetDeviceLocationName(&loc);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_console_timezone"), EncodeForSettings<std::string>(loc.name), -1);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_viewer_enabled"), EncodeForSettings(g_Config.viewer_usb_enabled), 1);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_flog_enabled"), EncodeForSettings(g_Config.system_title_override_enabled), 2);
        auto connectednet = cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_wifi_none");
        if(net::HasConnection()) {
            net::NetworkProfileData data = {};
            net::GetCurrentNetworkProfile(&data);
            connectednet = data.wifi_name;
        }
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_wifi_name"), EncodeForSettings(connectednet), 3);

        u64 lcode = 0;
        auto ilang = SetLanguage_ENUS;
        setGetLanguageCode(&lcode);
        setMakeLanguage(lcode, &ilang);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_console_lang"), EncodeForSettings(os::GetLanguageName(ilang)), 4);
        bool console_info_upload = false;
        setsysGetConsoleInformationUploadFlag(&console_info_upload);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_console_info_upload"), EncodeForSettings(console_info_upload), 5);
        bool auto_titles_dl = false;
        setsysGetAutomaticApplicationDownloadFlag(&auto_titles_dl);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_auto_titles_dl"), EncodeForSettings(auto_titles_dl), 6);
        bool auto_update = false;
        setsysGetAutoUpdateEnableFlag(&auto_update);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_auto_update"), EncodeForSettings(auto_update), 7);
        bool wireless_lan = false;
        setsysGetWirelessLanEnableFlag(&wireless_lan);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_wireless_lan"), EncodeForSettings(wireless_lan), 8);
        bool bluetooth = false;
        setsysGetBluetoothEnableFlag(&bluetooth);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_bluetooth"), EncodeForSettings(bluetooth), 9);
        bool usb_30 = false;
        setsysGetUsb30EnableFlag(&usb_30);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_usb_30"), EncodeForSettings(usb_30), 10);
        bool nfc = false;
        setsysGetNfcEnableFlag(&nfc);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_nfc"), EncodeForSettings(nfc), 11);
        SetSysSerialNumber serial = {};
        setsysGetSerialNumber(&serial);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_serial_no"), EncodeForSettings<std::string>(serial.number), -1);
        u64 mac = 0;
        net::GetMACAddress(&mac);
        auto strmac = net::FormatMACAddress(mac);
        this->PushSettingItem(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_mac_addr"), EncodeForSettings(strmac), -1);
        auto ipstr = net::GetConsoleIPAddress();
        // TODO: strings
        this->PushSettingItem("Console IP address", EncodeForSettings(ipstr), -1);
    }

    void SettingsMenuLayout::PushSettingItem(const std::string &name, const std::string &value_display, int id) {
        auto textclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        auto itm = pu::ui::elm::MenuItem::New(name + ": " + value_display);
        itm->AddOnClick(std::bind(&SettingsMenuLayout::setting_Click, this, id));
        itm->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/Setting" + std::string((id < 0) ? "No" : "") + "Editable.png"));
        itm->SetColor(textclr);
        this->settingsMenu->AddItem(itm);
    }

    void SettingsMenuLayout::setting_Click(u32 id) {
        bool reload_need = false;
        switch(id) {
            case 0: {
                SwkbdConfig swkbd;
                swkbdCreate(&swkbd, 0);
                swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "swkbd_console_nick_guide").c_str());
                SetSysDeviceNickName console_name = {};
                setsysGetDeviceNickname(&console_name);
                swkbdConfigSetInitialText(&swkbd, console_name.nickname);
                swkbdConfigSetStringLenMax(&swkbd, 32);
                SetSysDeviceNickName new_name = {};
                auto rc = swkbdShow(&swkbd, new_name.nickname, sizeof(new_name.nickname));
                swkbdClose(&swkbd);
                if(R_SUCCEEDED(rc)) {
                    setsysSetDeviceNickname(&new_name);
                    reload_need = true;
                }
                break;
            }
            case 1: {
                auto sopt = g_MenuApplication->CreateShowDialog(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_viewer_enabled"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_viewer_info") + "\n" + (g_Config.viewer_usb_enabled ? cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_disable_conf") : cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_enable_conf")), { cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "yes"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "cancel") }, true);
                if(sopt == 0) {
                    g_Config.viewer_usb_enabled = !g_Config.viewer_usb_enabled;
                    reload_need = true;
                    g_MenuApplication->ShowNotification(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_changed_reboot"));
                }
                break;
            }
            case 2: {
                auto sopt = g_MenuApplication->CreateShowDialog(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_flog_enabled"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_flog_info") + "\n" + (g_Config.viewer_usb_enabled ? cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_disable_conf") : cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "set_enable_conf")), { cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "yes"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "cancel") }, true);
                if(sopt == 0) {
                    g_Config.system_title_override_enabled = !g_Config.system_title_override_enabled;
                    reload_need = true;
                }
                break;
            }
            case 3: {
                u8 in[28] = {0};
                // 0 = normal, 1 = qlaunch, 2 = starter...?
                *reinterpret_cast<u32*>(in) = 1;
                u8 out[8] = {0};

                LibAppletArgs netargs;
                libappletArgsCreate(&netargs, 0);

                auto rc = libappletLaunch(AppletId_netConnect, &netargs, in, sizeof(in), out, sizeof(out), nullptr);
                if(R_SUCCEEDED(rc)) {
                    rc = *reinterpret_cast<Result*>(out);
                    if(R_SUCCEEDED(rc)) {
                        reload_need = true;
                    }
                }
                break;
            }
            case 4: {
                g_MenuApplication->FadeOut();
                g_MenuApplication->LoadSettingsLanguagesMenu();
                g_MenuApplication->FadeIn();

                break;
            }
            case 5: {
                auto console_info_upload = false;
                setsysGetConsoleInformationUploadFlag(&console_info_upload);
                setsysSetConsoleInformationUploadFlag(!console_info_upload);

                reload_need = true;
                break;
            }
            case 6: {
                auto auto_titles_dl = false;
                setsysGetAutomaticApplicationDownloadFlag(&auto_titles_dl);
                setsysSetAutomaticApplicationDownloadFlag(!auto_titles_dl);

                reload_need = true;
                break;
            }
            case 7: {
                auto auto_update = false;
                setsysGetAutoUpdateEnableFlag(&auto_update);
                setsysSetAutoUpdateEnableFlag(!auto_update);

                reload_need = true;
                break;
            }
            case 8: {
                auto wireless_lan = false;
                setsysGetWirelessLanEnableFlag(&wireless_lan);
                setsysSetWirelessLanEnableFlag(!wireless_lan);

                reload_need = true;
                break;
            }
            case 9: {
                auto bluetooth = false;
                setsysGetBluetoothEnableFlag(&bluetooth);
                setsysSetBluetoothEnableFlag(!bluetooth);

                reload_need = true;
                break;
            }
            case 10: {
                auto usb_30 = false;
                setsysGetUsb30EnableFlag(&usb_30);
                setsysSetUsb30EnableFlag(!usb_30);

                reload_need = true;
                break;
            }
            case 11: {
                auto nfc = false;
                setsysGetNfcEnableFlag(&nfc);
                setsysSetNfcEnableFlag(!nfc);

                reload_need = true;
                break;
            }
        }
        if(reload_need) {
            cfg::SaveConfig(g_Config);
            this->Reload();
        }
    }

}