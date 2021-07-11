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
        return GetLanguageString("set_unknown_value");
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
        return t ? GetLanguageString("set_true_value") : GetLanguageString("set_false_value");
    }

    SettingsMenuLayout::SettingsMenuLayout() {
        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));

        pu::ui::Color textclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        pu::ui::Color menufocusclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        pu::ui::Color menubgclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->infoText = pu::ui::elm::TextBlock::New(0, 100, GetLanguageString("set_info_text"));
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
        if(down & HidNpadButton_B) {
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
        this->PushSettingItem(GetLanguageString("set_console_nickname"), EncodeForSettings<std::string>(console_name.nickname), 0);
        TimeLocationName loc = {};
        timeGetDeviceLocationName(&loc);
        this->PushSettingItem(GetLanguageString("set_console_timezone"), EncodeForSettings<std::string>(loc.name), -1);
        bool viewer_usb_enabled;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::ViewerUsbEnabled, viewer_usb_enabled));
        this->PushSettingItem(GetLanguageString("set_viewer_enabled"), EncodeForSettings(viewer_usb_enabled), 1);
        auto connectednet = GetLanguageString("set_wifi_none");
        if(net::HasConnection()) {
            net::NetworkProfileData data = {};
            net::GetCurrentNetworkProfile(&data);
            connectednet = data.wifi_name;
        }
        this->PushSettingItem(GetLanguageString("set_wifi_name"), EncodeForSettings(connectednet), 2);

        u64 lcode = 0;
        auto ilang = SetLanguage_ENUS;
        setGetLanguageCode(&lcode);
        setMakeLanguage(lcode, &ilang);
        this->PushSettingItem(GetLanguageString("set_console_lang"), EncodeForSettings(os::GetLanguageName(ilang)), 3);
        bool console_info_upload = false;
        setsysGetConsoleInformationUploadFlag(&console_info_upload);
        this->PushSettingItem(GetLanguageString("set_console_info_upload"), EncodeForSettings(console_info_upload), 4);
        bool auto_titles_dl = false;
        setsysGetAutomaticApplicationDownloadFlag(&auto_titles_dl);
        this->PushSettingItem(GetLanguageString("set_auto_titles_dl"), EncodeForSettings(auto_titles_dl), 5);
        bool auto_update = false;
        setsysGetAutoUpdateEnableFlag(&auto_update);
        this->PushSettingItem(GetLanguageString("set_auto_update"), EncodeForSettings(auto_update), 6);
        bool wireless_lan = false;
        setsysGetWirelessLanEnableFlag(&wireless_lan);
        this->PushSettingItem(GetLanguageString("set_wireless_lan"), EncodeForSettings(wireless_lan), 7);
        bool bluetooth = false;
        setsysGetBluetoothEnableFlag(&bluetooth);
        this->PushSettingItem(GetLanguageString("set_bluetooth"), EncodeForSettings(bluetooth), 8);
        bool usb_30 = false;
        setsysGetUsb30EnableFlag(&usb_30);
        this->PushSettingItem(GetLanguageString("set_usb_30"), EncodeForSettings(usb_30), 9);
        bool nfc = false;
        setsysGetNfcEnableFlag(&nfc);
        this->PushSettingItem(GetLanguageString("set_nfc"), EncodeForSettings(nfc), 10);
        SetSysSerialNumber serial = {};
        setsysGetSerialNumber(&serial);
        this->PushSettingItem(GetLanguageString("set_serial_no"), EncodeForSettings<std::string>(serial.number), -1);
        u64 mac = 0;
        net::GetMACAddress(&mac);
        auto strmac = net::FormatMACAddress(mac);
        this->PushSettingItem(GetLanguageString("set_mac_addr"), EncodeForSettings(strmac), -1);
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
                swkbdConfigSetGuideText(&swkbd, GetLanguageString("swkbd_console_nick_guide").c_str());
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
                bool viewer_usb_enabled;
                UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::ViewerUsbEnabled, viewer_usb_enabled));
                auto sopt = g_MenuApplication->CreateShowDialog(GetLanguageString("set_viewer_enabled"), GetLanguageString("set_viewer_info") + "\n" + (viewer_usb_enabled ? GetLanguageString("set_disable_conf") : GetLanguageString("set_enable_conf")), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                if(sopt == 0) {
                    viewer_usb_enabled = !viewer_usb_enabled;
                    UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::ViewerUsbEnabled, viewer_usb_enabled));
                    reload_need = true;
                    g_MenuApplication->ShowNotification(GetLanguageString("set_changed_reboot"));
                }
                break;
            }
            case 2: {
                u8 in[28] = {0};
                // 0 = normal, 1 = qlaunch, 2 = starter...?
                *reinterpret_cast<u32*>(in) = 1;
                u8 out[8] = {0};

                LibAppletArgs netargs;
                libappletArgsCreate(&netargs, 0);

                auto rc = libappletLaunch(AppletId_LibraryAppletNetConnect, &netargs, in, sizeof(in), out, sizeof(out), nullptr);
                if(R_SUCCEEDED(rc)) {
                    rc = *reinterpret_cast<Result*>(out);
                    if(R_SUCCEEDED(rc)) {
                        reload_need = true;
                    }
                }
                break;
            }
            case 3: {
                g_MenuApplication->FadeOut();
                g_MenuApplication->LoadSettingsLanguagesMenu();
                g_MenuApplication->FadeIn();

                break;
            }
            case 4: {
                auto console_info_upload = false;
                setsysGetConsoleInformationUploadFlag(&console_info_upload);
                setsysSetConsoleInformationUploadFlag(!console_info_upload);

                reload_need = true;
                break;
            }
            case 5: {
                auto auto_titles_dl = false;
                setsysGetAutomaticApplicationDownloadFlag(&auto_titles_dl);
                setsysSetAutomaticApplicationDownloadFlag(!auto_titles_dl);

                reload_need = true;
                break;
            }
            case 6: {
                auto auto_update = false;
                setsysGetAutoUpdateEnableFlag(&auto_update);
                setsysSetAutoUpdateEnableFlag(!auto_update);

                reload_need = true;
                break;
            }
            case 7: {
                auto wireless_lan = false;
                setsysGetWirelessLanEnableFlag(&wireless_lan);
                setsysSetWirelessLanEnableFlag(!wireless_lan);

                reload_need = true;
                break;
            }
            case 8: {
                auto bluetooth = false;
                setsysGetBluetoothEnableFlag(&bluetooth);
                setsysSetBluetoothEnableFlag(!bluetooth);

                reload_need = true;
                break;
            }
            case 9: {
                auto usb_30 = false;
                setsysGetUsb30EnableFlag(&usb_30);
                setsysSetUsb30EnableFlag(!usb_30);

                reload_need = true;
                break;
            }
            case 10: {
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