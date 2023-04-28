#include <ui/ui_SettingsMenuLayout.hpp>
#include <os/os_Account.hpp>
#include <os/os_Misc.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <fs/fs_Stdio.hpp>
#include <net/net_Service.hpp>
#include <am/am_LibraryApplet.hpp>

extern ui::MenuApplication::Ref g_MenuApplication;
extern ui::TransitionGuard g_TransitionGuard;
extern cfg::Theme g_Theme;
extern cfg::Config g_Config;

namespace ui {
    
    template<typename T>
    inline std::string EncodeForSettings(const T &t) {
        return GetLanguageString("set_unknown_value");
    }

    template<>
    inline std::string EncodeForSettings<std::string>(const std::string &t) {
        return "\"" + t + "\"";
    }
    
    template<>
    inline std::string EncodeForSettings<u32>(const u32 &t) {
        return "\"" + std::to_string(t) + "\"";
    }

    template<>
    inline std::string EncodeForSettings<bool>(const bool &t) {
        return t ? GetLanguageString("set_true_value") : GetLanguageString("set_false_value");
    }

    SettingsMenuLayout::SettingsMenuLayout() {
        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));

        this->info_text = pu::ui::elm::TextBlock::New(0, 25, GetLanguageString("set_info_text"));
        this->info_text->SetColor(g_MenuApplication->GetTextColor());
        this->info_text->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        g_MenuApplication->ApplyConfigForElement("settings_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->settings_menu = pu::ui::elm::Menu::New(50, 80, 1180, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), 100, 6);
        g_MenuApplication->ApplyConfigForElement("settings_menu", "settings_menu_item", this->settings_menu);
        this->Add(this->settings_menu);

        this->settings_scroll_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/SettingsScroll.wav"));
        this->settings_back_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/SettingsBack.wav"));

        this->settings_back_sfx_played=false;

    }

    void SettingsMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(keys_down & HidNpadButton_B) {
            this->settings_back_sfx_played=true;
            pu::audio::PlaySfx(this->settings_back_sfx); //When i am going back to the main menu from the settings menu i want to play the sfx
            g_TransitionGuard.Run([]() {
                g_MenuApplication->FadeOut();
                g_MenuApplication->LoadMenu();
                g_MenuApplication->FadeIn();
            });
        }else if(keys_down & HidNpadButton_AnyUp || keys_down & HidNpadButton_AnyDown){
            pu::audio::PlaySfx(this->settings_scroll_sfx); //When scrolling the settings i want to play the sfx
        }
    }

    bool SettingsMenuLayout::OnHomeButtonPress() {
        if(!this->settings_back_sfx_played){
            this->settings_back_sfx_played=true;
            pu::audio::PlaySfx(this->settings_back_sfx); //When i am going back to the main menu from the themes menu i want to play the sfx
        }
        return g_TransitionGuard.Run([]() {
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadMenu();
            g_MenuApplication->FadeIn();
        });
    }

    void SettingsMenuLayout::Reload(const bool reset_idx) {
        // TODO: more settings

        this->settings_back_sfx_played=false; //Need to re-enable the previously deactivated sfx
        this->settings_menu->ClearItems();
        
        SetSysDeviceNickName console_name = {};
        setsysGetDeviceNickname(&console_name);
        this->PushSettingItem(GetLanguageString("set_console_nickname"), EncodeForSettings<std::string>(console_name.nickname), 0);
        
        TimeLocationName loc = {};
        timeGetDeviceLocationName(&loc);
        this->PushSettingItem(GetLanguageString("set_console_timezone"), EncodeForSettings<std::string>(loc.name), -1);
        
        bool viewer_usb_enabled;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::ViewerUsbEnabled, viewer_usb_enabled));
        this->PushSettingItem(GetLanguageString("set_viewer_enabled"), EncodeForSettings(viewer_usb_enabled), 1);
        
        auto connected_wifi_name = GetLanguageString("set_wifi_none");
        if(net::HasConnection()) {
            net::NetworkProfileData prof_data = {};
            net::GetCurrentNetworkProfile(prof_data);
            connected_wifi_name = prof_data.wifi_name;
        }
        this->PushSettingItem(GetLanguageString("set_wifi_name"), EncodeForSettings(connected_wifi_name), 2);

        u64 lang_code = 0;
        auto lang_val = SetLanguage_ENUS;
        setGetSystemLanguage(&lang_code);
        setMakeLanguage(lang_code, &lang_val);
        const std::string lang_str = os::LanguageNameList[static_cast<u32>(lang_val)];
        this->PushSettingItem(GetLanguageString("set_console_lang"), EncodeForSettings(lang_str), 3);

        auto console_info_upload = false;
        setsysGetConsoleInformationUploadFlag(&console_info_upload);
        this->PushSettingItem(GetLanguageString("set_console_info_upload"), EncodeForSettings(console_info_upload), 4);
        
        auto auto_titles_dl = false;
        setsysGetAutomaticApplicationDownloadFlag(&auto_titles_dl);
        this->PushSettingItem(GetLanguageString("set_auto_titles_dl"), EncodeForSettings(auto_titles_dl), 5);
        
        auto auto_update = false;
        setsysGetAutoUpdateEnableFlag(&auto_update);
        this->PushSettingItem(GetLanguageString("set_auto_update"), EncodeForSettings(auto_update), 6);
        
        auto wireless_lan = false;
        setsysGetWirelessLanEnableFlag(&wireless_lan);
        this->PushSettingItem(GetLanguageString("set_wireless_lan"), EncodeForSettings(wireless_lan), 7);
        
        auto bluetooth = false;
        setsysGetBluetoothEnableFlag(&bluetooth);
        this->PushSettingItem(GetLanguageString("set_bluetooth"), EncodeForSettings(bluetooth), 8);
        
        auto usb_30 = false;
        setsysGetUsb30EnableFlag(&usb_30);
        this->PushSettingItem(GetLanguageString("set_usb_30"), EncodeForSettings(usb_30), 9);
        
        auto nfc = false;
        setsysGetNfcEnableFlag(&nfc);
        this->PushSettingItem(GetLanguageString("set_nfc"), EncodeForSettings(nfc), 10);
        
        SetSysSerialNumber serial = {};
        setsysGetSerialNumber(&serial);
        this->PushSettingItem(GetLanguageString("set_serial_no"), EncodeForSettings<std::string>(serial.number), -1);
        
        net::WlanMacAddress mac_addr = {};
        net::GetMacAddress(mac_addr);
        const auto mac_addr_str = net::FormatMacAddress(mac_addr);
        this->PushSettingItem(GetLanguageString("set_mac_addr"), EncodeForSettings(mac_addr_str), -1);

        const auto ip_str = net::GetConsoleIpAddress();
        this->PushSettingItem("Console IP address", EncodeForSettings(ip_str), -1);

        if(reset_idx) {
            this->settings_menu->SetSelectedIndex(0);
        }
    }

    void SettingsMenuLayout::PushSettingItem(const std::string &name, const std::string &value_display, int id) {
        const auto is_editable = id >= 0;
        auto setting_item = pu::ui::elm::MenuItem::New(name + ": " + value_display);
        if(is_editable) {
            setting_item->AddOnKey(std::bind(&SettingsMenuLayout::setting_DefaultKey, this, id));
        }
        setting_item->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/Setting" + std::string(is_editable ? "" : "No") + "Editable.png"));
        setting_item->SetColor(g_MenuApplication->GetTextColor());
        this->settings_menu->AddItem(setting_item);
    }

    void SettingsMenuLayout::setting_DefaultKey(const u32 id) {
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
                g_TransitionGuard.Run([]() {
                    g_MenuApplication->FadeOut();
                    g_MenuApplication->LoadSettingsLanguagesMenu();
                    g_MenuApplication->FadeIn();
                });

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
            this->Reload(false);
        }
    }

}