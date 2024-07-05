#include <ul/menu/ui/ui_SettingsMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/net/net_Service.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/os/os_System.hpp>
#include <ul/util/util_Scope.hpp>

extern SetSysFirmwareVersion g_FwVersion;

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::cfg::Config g_Config;

namespace ul::menu::ui {

    namespace {

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

        constexpr u32 ExosphereApiVersionConfigItem = 65000;
        constexpr u32 ExosphereEmummcType = 65007;
        constexpr u32 ExosphereSupportedHosVersion = 65011;

        bool g_AmsEmummcInfoLoaded = false;
        ul::Version g_AmsVersion;
        bool g_AmsIsEmummc;

        void LoadAmsEmummcInfo() {
            if(!g_AmsEmummcInfoLoaded) {
                UL_RC_ASSERT(splInitialize());
                UL_ON_SCOPE_EXIT(
                    splExit();
                );

                // Since we rely on ams for uLaunch to work, it *must* be present

                u64 raw_ams_ver;
                UL_RC_ASSERT(splGetConfig(static_cast<SplConfigItem>(ExosphereApiVersionConfigItem), &raw_ams_ver));
                g_AmsVersion = {
                    .major = static_cast<u8>((raw_ams_ver >> 56) & 0xFF),
                    .minor = static_cast<u8>((raw_ams_ver >> 48) & 0xFF),
                    .micro = static_cast<u8>((raw_ams_ver >> 40) & 0xFF)
                };

                u64 emummc_type;
                UL_RC_ASSERT(splGetConfig(static_cast<SplConfigItem>(ExosphereEmummcType), &emummc_type));
                g_AmsIsEmummc = emummc_type != 0;
                g_AmsEmummcInfoLoaded = true;
            }
        }

    }

    SettingsMenuLayout::SettingsMenuLayout() : IMenuLayout() {
        LoadAmsEmummcInfo();

        this->info_text = pu::ui::elm::TextBlock::New(0, 0, GetLanguageString("set_info_text"));

        this->info_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("settings_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->settings_menu = pu::ui::elm::Menu::New(0, 0, SettingsMenuWidth, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), SettingsMenuItemSize, SettingsMenuItemsToShow);
        g_MenuApplication->ApplyConfigForElement("settings_menu", "settings_menu", this->settings_menu);
        this->Add(this->settings_menu);

        this->setting_edit_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Settings/SettingEdit.wav"));
        this->setting_save_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Settings/SettingSave.wav"));
        this->back_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Settings/Back.wav"));
    }

    void SettingsMenuLayout::DisposeAudio() {
        pu::audio::DestroySfx(this->setting_edit_sfx);
        pu::audio::DestroySfx(this->setting_save_sfx);
        pu::audio::DestroySfx(this->back_sfx);
    }

    void SettingsMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(keys_down & HidNpadButton_B) {
            pu::audio::PlaySfx(this->back_sfx);

            g_MenuApplication->LoadMenuByType(MenuType::Main);
        }
    }

    bool SettingsMenuLayout::OnHomeButtonPress() {
        pu::audio::PlaySfx(this->back_sfx);

        g_MenuApplication->LoadMenuByType(MenuType::Main);
        return true;
    }

    void SettingsMenuLayout::Reload(const bool reset_idx) {
        // TODO (long term): implement more settings!

        const auto prev_idx = this->settings_menu->GetSelectedIndex();
        this->settings_menu->ClearItems();

        this->PushSettingItem(GetLanguageString("set_console_fw"), EncodeForSettings<std::string>(std::string(g_FwVersion.display_version) + " (" + g_FwVersion.display_title + ")"), -1);

        this->PushSettingItem(GetLanguageString("set_ams_fw"), EncodeForSettings<std::string>(g_AmsVersion.Format()), -1);

        this->PushSettingItem(GetLanguageString("set_ams_emummc"), EncodeForSettings(g_AmsIsEmummc), -1);
        
        SetSysDeviceNickName console_name = {};
        UL_RC_ASSERT(setsysGetDeviceNickname(&console_name));
        this->PushSettingItem(GetLanguageString("set_console_nickname"), EncodeForSettings<std::string>(console_name.nickname), 0);
        
        TimeLocationName loc = {};
        UL_RC_ASSERT(timeGetDeviceLocationName(&loc));
        this->PushSettingItem(GetLanguageString("set_console_timezone"), EncodeForSettings<std::string>(loc.name), -1);

        bool viewer_usb_enabled;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::ViewerUsbEnabled, viewer_usb_enabled));
        this->PushSettingItem(GetLanguageString("set_viewer_enabled"), EncodeForSettings(viewer_usb_enabled), 1);

        auto connected_wifi_name = GetLanguageString("set_wifi_none");
        u32 strength;
        if(net::HasConnection(strength)) {
            NifmNetworkProfileData prof_data = {};
            if(R_SUCCEEDED(nifmGetCurrentNetworkProfile(&prof_data))) {
                connected_wifi_name = prof_data.wireless_setting_data.ssid;
            }
        }
        this->PushSettingItem(GetLanguageString("set_wifi_name"), EncodeForSettings(connected_wifi_name), 2);

        u64 lang_code = 0;
        auto lang_val = SetLanguage_ENUS;
        UL_RC_ASSERT(setGetSystemLanguage(&lang_code));
        UL_RC_ASSERT(setMakeLanguage(lang_code, &lang_val));
        const std::string lang_str = os::LanguageNameList[static_cast<u32>(lang_val)];
        this->PushSettingItem(GetLanguageString("set_console_lang"), EncodeForSettings(lang_str), 3);

        auto console_info_upload = false;
        UL_RC_ASSERT(setsysGetConsoleInformationUploadFlag(&console_info_upload));
        this->PushSettingItem(GetLanguageString("set_console_info_upload"), EncodeForSettings(console_info_upload), 4);
        
        auto auto_titles_dl = false;
        UL_RC_ASSERT(setsysGetAutomaticApplicationDownloadFlag(&auto_titles_dl));
        this->PushSettingItem(GetLanguageString("set_auto_titles_dl"), EncodeForSettings(auto_titles_dl), 5);
        
        auto auto_update = false;
        UL_RC_ASSERT(setsysGetAutoUpdateEnableFlag(&auto_update));
        this->PushSettingItem(GetLanguageString("set_auto_update"), EncodeForSettings(auto_update), 6);
        
        auto wireless_lan = false;
        UL_RC_ASSERT(setsysGetWirelessLanEnableFlag(&wireless_lan));
        this->PushSettingItem(GetLanguageString("set_wireless_lan"), EncodeForSettings(wireless_lan), 7);
        
        auto bluetooth = false;
        UL_RC_ASSERT(setsysGetBluetoothEnableFlag(&bluetooth));
        this->PushSettingItem(GetLanguageString("set_bluetooth"), EncodeForSettings(bluetooth), 8);
        
        auto usb_30 = false;
        UL_RC_ASSERT(setsysGetUsb30EnableFlag(&usb_30));
        this->PushSettingItem(GetLanguageString("set_usb_30"), EncodeForSettings(usb_30), 9);
        
        auto nfc = false;
        UL_RC_ASSERT(setsysGetNfcEnableFlag(&nfc));
        this->PushSettingItem(GetLanguageString("set_nfc"), EncodeForSettings(nfc), 10);
        
        SetSysSerialNumber serial = {};
        UL_RC_ASSERT(setsysGetSerialNumber(&serial));
        this->PushSettingItem(GetLanguageString("set_serial_no"), EncodeForSettings<std::string>(serial.number), -1);
        
        net::WlanMacAddress mac_addr = {};
        UL_RC_ASSERT(net::GetMacAddress(mac_addr));
        const auto mac_addr_str = net::FormatMacAddress(mac_addr);
        this->PushSettingItem(GetLanguageString("set_mac_addr"), EncodeForSettings(mac_addr_str), -1);

        const auto ip_str = net::GetConsoleIpAddress();
        this->PushSettingItem(GetLanguageString("set_ip_addr"), EncodeForSettings(ip_str), -1);

        this->settings_menu->SetSelectedIndex(reset_idx ? 0 : prev_idx);
    }

    void SettingsMenuLayout::PushSettingItem(const std::string &name, const std::string &value_display, int id) {
        const auto is_editable = id >= 0;
        auto setting_item = pu::ui::elm::MenuItem::New(name + ": " + value_display);
        if(is_editable) {
            setting_item->AddOnKey(std::bind(&SettingsMenuLayout::setting_DefaultKey, this, id));
        }
        setting_item->SetIcon(is_editable ? GetEditableSettingIconTexture() : GetNonEditableSettingIconTexture());
        setting_item->SetColor(g_MenuApplication->GetTextColor());
        this->settings_menu->AddItem(setting_item);
    }

    void SettingsMenuLayout::setting_DefaultKey(const u32 id) {
        pu::audio::PlaySfx(this->setting_edit_sfx);
        bool reload_need = false;
        switch(id) {
            case 0: {
                SwkbdConfig swkbd;
                if(R_SUCCEEDED(swkbdCreate(&swkbd, 0))) {
                    swkbdConfigSetGuideText(&swkbd, GetLanguageString("swkbd_console_nick_guide").c_str());
                    SetSysDeviceNickName console_name = {};
                    UL_RC_ASSERT(setsysGetDeviceNickname(&console_name));
                    swkbdConfigSetInitialText(&swkbd, console_name.nickname);
                    swkbdConfigSetStringLenMax(&swkbd, 32);
                    SetSysDeviceNickName new_name = {};
                    auto rc = swkbdShow(&swkbd, new_name.nickname, sizeof(new_name.nickname));
                    swkbdClose(&swkbd);
                    if(R_SUCCEEDED(rc)) {
                        setsysSetDeviceNickname(&new_name);
                        reload_need = true;
                    }
                }
                break;
            }
            case 1: {
                bool viewer_usb_enabled;
                UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::ViewerUsbEnabled, viewer_usb_enabled));
                auto sopt = g_MenuApplication->DisplayDialog(GetLanguageString("set_viewer_enabled"), GetLanguageString("set_viewer_info") + "\n" + (viewer_usb_enabled ? GetLanguageString("set_viewer_disable_conf") : GetLanguageString("set_viewer_enable_conf")), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                if(sopt == 0) {
                    viewer_usb_enabled = !viewer_usb_enabled;
                    UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::ViewerUsbEnabled, viewer_usb_enabled));
                    reload_need = true;
                    g_MenuApplication->ShowNotification(GetLanguageString("set_changed_reboot"));
                }
                break;
            }
            case 2: {
                // Close uMenu and launch the applet
                ShowNetConnect();
                break;
            }
            case 3: {
                std::vector<std::string> lang_opts = {};
                for(u32 i = 0; i < os::LanguageNameCount; i++) {
                    lang_opts.push_back(os::LanguageNameList[i]);
                }
                lang_opts.push_back(GetLanguageString("cancel"));

                const auto opt = g_MenuApplication->DisplayDialog(GetLanguageString("set_lang_select"), GetLanguageString("set_lang_conf"), lang_opts, true);
                if(opt >= 0) {
                    const auto sys_lang = os::GetSystemLanguage();
                    if(sys_lang == opt) {
                        g_MenuApplication->ShowNotification(GetLanguageString("set_lang_active"));
                    }
                    else {
                        u64 lang_codes[os::LanguageNameCount] = {};
                        s32 tmp;
                        setGetAvailableLanguageCodes(&tmp, lang_codes, os::LanguageNameCount);
                        const auto lang_code = lang_codes[opt];

                        const auto rc = setsysSetLanguageCode(lang_code);
                        g_MenuApplication->DisplayDialog(GetLanguageString("set_lang"), R_SUCCEEDED(rc) ? GetLanguageString("set_lang_select_ok") : GetLanguageString("set_lang_select_error") + ": " + util::FormatResultDisplay(rc), { GetLanguageString("ok") }, true);
                        if(R_SUCCEEDED(rc)) {
                            RebootSystem();
                        }
                    }
                }
                break;
            }
            case 4: {
                auto console_info_upload = false;
                UL_RC_ASSERT(setsysGetConsoleInformationUploadFlag(&console_info_upload));
                UL_RC_ASSERT(setsysSetConsoleInformationUploadFlag(!console_info_upload));

                reload_need = true;
                break;
            }
            case 5: {
                auto auto_app_dl = false;
                UL_RC_ASSERT(setsysGetAutomaticApplicationDownloadFlag(&auto_app_dl));
                UL_RC_ASSERT(setsysSetAutomaticApplicationDownloadFlag(!auto_app_dl));

                reload_need = true;
                break;
            }
            case 6: {
                auto auto_update = false;
                UL_RC_ASSERT(setsysGetAutoUpdateEnableFlag(&auto_update));
                UL_RC_ASSERT(setsysSetAutoUpdateEnableFlag(!auto_update));

                reload_need = true;
                break;
            }
            case 7: {
                auto wireless_lan = false;
                UL_RC_ASSERT(setsysGetWirelessLanEnableFlag(&wireless_lan));
                UL_RC_ASSERT(setsysSetWirelessLanEnableFlag(!wireless_lan));

                reload_need = true;
                break;
            }
            case 8: {
                auto bluetooth = false;
                UL_RC_ASSERT(setsysGetBluetoothEnableFlag(&bluetooth));
                UL_RC_ASSERT(setsysSetBluetoothEnableFlag(!bluetooth));

                reload_need = true;
                break;
            }
            case 9: {
                auto usb_30 = false;
                UL_RC_ASSERT(setsysGetUsb30EnableFlag(&usb_30));
                UL_RC_ASSERT(setsysSetUsb30EnableFlag(!usb_30));

                reload_need = true;
                break;
            }
            case 10: {
                auto nfc = false;
                UL_RC_ASSERT(setsysGetNfcEnableFlag(&nfc));
                UL_RC_ASSERT(setsysSetNfcEnableFlag(!nfc));

                reload_need = true;
                break;
            }
        }

        if(reload_need) {
            pu::audio::PlaySfx(this->setting_save_sfx);
            SaveConfig();
            this->Reload(false);
        }
    }

}
