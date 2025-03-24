#include <ul/menu/ui/ui_SettingsMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/net/net_Service.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/util/util_Scope.hpp>
#include <malloc.h>

extern ul::menu::ui::GlobalSettings g_GlobalSettings;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    namespace {

        std::string GetMenuName(const SettingMenu menu) {
            switch(menu) {
                case SettingMenu::System: return GetLanguageString("set_menu_system");
                case SettingMenu::uLaunch: return GetLanguageString("set_menu_ulaunch");
                case SettingMenu::Bluetooth: return GetLanguageString("set_menu_bluetooth");
                case SettingMenu::Network: return GetLanguageString("set_menu_network");
                case SettingMenu::Screen: return GetLanguageString("set_menu_screen");
                case SettingMenu::Dev: return GetLanguageString("set_menu_dev");

                default: return GetLanguageString("set_unknown_value");
            }
        }

        inline std::string FormatBoolean(const bool &t) {
            return t ? GetLanguageString("set_true_value") : GetLanguageString("set_false_value");
        }

        inline std::string FormatRegion(const SetRegion region) {
            switch(region) {
                case SetRegion_JPN: return GetLanguageString("set_region_jpn");
                case SetRegion_USA: return GetLanguageString("set_region_usa");
                case SetRegion_EUR: return GetLanguageString("set_region_eur");
                case SetRegion_AUS: return GetLanguageString("set_region_aus");
                case SetRegion_HTK: return GetLanguageString("set_region_htk");
                case SetRegion_CHN: return GetLanguageString("set_region_chn");
                default: return GetLanguageString("set_unknown_value");
            }
        }

        inline Result GetLatestSystemUpdate(u8 &out_value) {
            NsSystemUpdateControl ctrl;
            UL_RC_TRY(nssuOpenSystemUpdateControl(&ctrl));
            UL_ON_SCOPE_EXIT({
                nssuControlClose(&ctrl);
            });

            AsyncValue av;
            UL_RC_TRY(nssuControlRequestCheckLatestUpdate(&ctrl, &av));
            UL_ON_SCOPE_EXIT({
                asyncValueClose(&av);
            });

            // Just try waiting half a second
            UL_RC_TRY(asyncValueWait(&av, 500'000'000));

            u8 latest_upd_value;
            UL_RC_TRY(asyncValueGet(&av, &latest_upd_value, sizeof(latest_upd_value)));

            out_value = latest_upd_value;
            UL_RC_SUCCEED;
        }

        inline std::string FormatLatestSystemUpdate(const u8 upd) {
            switch(upd) {
                case 0: return GetLanguageString("set_update_updated");
                case 1: return GetLanguageString("set_update_ready_install");
                case 2: return GetLanguageString("set_update_needs_download");
                default: return GetLanguageString("set_unknown_value");
            }
        }

        inline std::string FormatPrimaryAlbumStorage(const SetSysPrimaryAlbumStorage storage) {
            switch(storage) {
                case SetSysPrimaryAlbumStorage_Nand: return GetLanguageString("set_album_nand");
                case SetSysPrimaryAlbumStorage_SdCard: return GetLanguageString("set_album_sd");
                default: return GetLanguageString("set_unknown_value");
            }
        }

        inline std::string FormatConsoleSleepPlan(const SetSysConsoleSleepPlan plan) {
            switch(plan) {
                case SetSysConsoleSleepPlan_1Hour: return GetLanguageString("set_sleep_console_1h");
                case SetSysConsoleSleepPlan_2Hour: return GetLanguageString("set_sleep_console_2h");
                case SetSysConsoleSleepPlan_3Hour: return GetLanguageString("set_sleep_console_3h");
                case SetSysConsoleSleepPlan_6Hour: return GetLanguageString("set_sleep_console_6h");
                case SetSysConsoleSleepPlan_12Hour: return GetLanguageString("set_sleep_console_12h");
                case SetSysConsoleSleepPlan_Never: return GetLanguageString("set_sleep_console_never");
                default: return GetLanguageString("set_unknown_value");
            }
        }

        inline std::string FormatHandheldSleepPlan(const SetSysHandheldSleepPlan plan) {
            switch(plan) {
                case SetSysHandheldSleepPlan_1Min: return GetLanguageString("set_sleep_handheld_1min");
                case SetSysHandheldSleepPlan_3Min: return GetLanguageString("set_sleep_handheld_3min");
                case SetSysHandheldSleepPlan_5Min: return GetLanguageString("set_sleep_handheld_5min");
                case SetSysHandheldSleepPlan_10Min: return GetLanguageString("set_sleep_handheld_10min");
                case SetSysHandheldSleepPlan_30Min: return GetLanguageString("set_sleep_handheld_30min");
                case SetSysHandheldSleepPlan_Never: return GetLanguageString("set_sleep_handheld_never");
                default: return GetLanguageString("set_unknown_value");
            }
        }

        inline std::string GetAudioService() {
            // No need to translate these
            if(serviceIsActive(audrenGetServiceSession_AudioRenderer())) {
                return "audren";
            }
            else if(serviceIsActive(audoutGetServiceSession_AudioOut())) {
                return "audout";
            }
            else {
                return GetLanguageString("set_unknown_value");
            }
        }

        constexpr auto SleepFlag_SleepsWhilePlayingMedia = BIT(0);
        constexpr auto SleepFlag_WakesAtPowerStateChange = BIT(1);

        struct SettingInfo {
            SettingMenu menu;
            SettingSubmenu submenu;
            Setting setting;
            bool editable;
            pu::ui::elm::MenuItem::Ref menu_item;

            std::string static_description;
            std::function<std::string()> dyn_description_cb;

            std::string static_value;
            std::function<std::string()> dyn_value_cb;

            std::function<bool()> select_cb;

            inline std::string GetDescription() {
                if(!this->static_description.empty()) {
                    return this->static_description;
                }
                else if(this->dyn_description_cb != nullptr) {
                    return this->dyn_description_cb();
                }
                else {
                    return "";
                }
            }

            inline std::string GetValue() {
                if(!this->static_value.empty()) {
                    return this->static_value;
                }
                else {
                    return this->dyn_value_cb();
                }
            }

            inline std::string Format() {
                const auto &desc = this->GetDescription();
                const auto &val = this->GetValue();
                if(desc.empty()) {
                    return val;
                }
                else {
                    return desc + ": " + val;
                }
            }
        };

        std::vector<SettingInfo> g_Settings;

        void LoadSettings() {
            g_Settings.clear();

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConsoleRegion,
                .editable = false,
                .static_description = GetLanguageString("set_region"),
                .static_value = FormatRegion(g_GlobalSettings.region)
            });

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::SystemUpdateStatus,
                .editable = false,
                .static_description = GetLanguageString("set_update"),
                .dyn_value_cb = []() {
                    u8 latest_upd_value = 0xFF;
                    if(R_SUCCEEDED(GetLatestSystemUpdate(latest_upd_value))) {
                        return FormatLatestSystemUpdate(latest_upd_value);
                    }
                    else {
                        return GetLanguageString("set_update_no_connection");
                    }
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Screen,
                .submenu = SettingSubmenu::None,
                .setting = Setting::LockscreenEnabled,
                .editable = true,
                .static_description = GetLanguageString("set_lockscreen"),
                .dyn_value_cb = []() {
                    bool lockscreen_enabled;
                    UL_ASSERT_TRUE(g_GlobalSettings.config.GetEntry(cfg::ConfigEntryId::LockscreenEnabled, lockscreen_enabled));
                    return FormatBoolean(lockscreen_enabled);
                },
                .select_cb = []() {
                    bool lockscreen_enabled;
                    UL_ASSERT_TRUE(g_GlobalSettings.config.GetEntry(cfg::ConfigEntryId::LockscreenEnabled, lockscreen_enabled));
                    UL_ASSERT_TRUE(g_GlobalSettings.config.SetEntry(cfg::ConfigEntryId::LockscreenEnabled, !lockscreen_enabled));
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Screen,
                .submenu = SettingSubmenu::None,
                .setting = Setting::PrimaryAlbumStorage,
                .editable = true,
                .static_description = GetLanguageString("set_album"),
                .dyn_value_cb = []() {
                    return FormatPrimaryAlbumStorage(g_GlobalSettings.album_storage);
                },
                .select_cb = []() {
                    const auto opt = g_MenuApplication->DisplayDialog(GetLanguageString("set_album"), GetLanguageString("set_enum_options"), {
                        GetLanguageString("set_album_sd"),
                        GetLanguageString("set_album_nand"),
                        GetLanguageString("cancel")
                    }, true);
                    switch(opt) {
                        case 0:
                            g_GlobalSettings.album_storage = SetSysPrimaryAlbumStorage_SdCard;
                            UL_RC_ASSERT(setsysSetPrimaryAlbumStorage(g_GlobalSettings.album_storage));
                            return true;
                        case 1:
                            g_GlobalSettings.album_storage = SetSysPrimaryAlbumStorage_Nand;
                            UL_RC_ASSERT(setsysSetPrimaryAlbumStorage(g_GlobalSettings.album_storage));
                            return true;
                    }
                    return false;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Screen,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConsoleSleepPlan,
                .editable = true,
                .static_description = GetLanguageString("set_sleep_console"),
                .dyn_value_cb = []() {
                    return FormatConsoleSleepPlan((SetSysConsoleSleepPlan)g_GlobalSettings.sleep_settings.console_sleep_plan);
                },
                .select_cb = []() {
                    const auto opt = g_MenuApplication->DisplayDialog(GetLanguageString("set_sleep_console"), GetLanguageString("set_enum_option"), {
                        GetLanguageString("set_sleep_console_never"),
                        GetLanguageString("set_sleep_console_1h"),
                        GetLanguageString("set_sleep_console_2h"),
                        GetLanguageString("set_sleep_console_3h"),
                        GetLanguageString("set_sleep_console_6h"),
                        GetLanguageString("set_sleep_console_12h"),
                        GetLanguageString("cancel")
                    }, true);
                    switch(opt) {
                        case 0:
                            g_GlobalSettings.sleep_settings.console_sleep_plan = SetSysConsoleSleepPlan_Never;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 1:
                            g_GlobalSettings.sleep_settings.console_sleep_plan = SetSysConsoleSleepPlan_1Hour;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 2:
                            g_GlobalSettings.sleep_settings.console_sleep_plan = SetSysConsoleSleepPlan_2Hour;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 3:
                            g_GlobalSettings.sleep_settings.console_sleep_plan = SetSysConsoleSleepPlan_3Hour;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 4:
                            g_GlobalSettings.sleep_settings.console_sleep_plan = SetSysConsoleSleepPlan_6Hour;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 5:
                            g_GlobalSettings.sleep_settings.console_sleep_plan = SetSysConsoleSleepPlan_12Hour;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                    }
                    return false;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Screen,
                .submenu = SettingSubmenu::None,
                .setting = Setting::HandheldSleepPlan,
                .editable = true,
                .static_description = GetLanguageString("set_sleep_handheld"),
                .dyn_value_cb = []() {
                    return FormatHandheldSleepPlan((SetSysHandheldSleepPlan)g_GlobalSettings.sleep_settings.handheld_sleep_plan);
                },
                .select_cb = []() {
                    const auto opt = g_MenuApplication->DisplayDialog(GetLanguageString("set_sleep_handheld"), GetLanguageString("set_enum_options"), {
                        GetLanguageString("set_sleep_handheld_never"),
                        GetLanguageString("set_sleep_handheld_1min"),
                        GetLanguageString("set_sleep_handheld_3min"),
                        GetLanguageString("set_sleep_handheld_5min"),
                        GetLanguageString("set_sleep_handheld_10min"),
                        GetLanguageString("set_sleep_handheld_30min"),
                        GetLanguageString("cancel")
                    }, true);
                    switch(opt) {
                        case 0:
                            g_GlobalSettings.sleep_settings.handheld_sleep_plan = SetSysHandheldSleepPlan_Never;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 1:
                            g_GlobalSettings.sleep_settings.handheld_sleep_plan = SetSysHandheldSleepPlan_1Min;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 2:
                            g_GlobalSettings.sleep_settings.handheld_sleep_plan = SetSysHandheldSleepPlan_3Min;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 3:
                            g_GlobalSettings.sleep_settings.handheld_sleep_plan = SetSysHandheldSleepPlan_5Min;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 4:
                            g_GlobalSettings.sleep_settings.handheld_sleep_plan = SetSysHandheldSleepPlan_10Min;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                        case 5:
                            g_GlobalSettings.sleep_settings.handheld_sleep_plan = SetSysHandheldSleepPlan_30Min;
                            g_GlobalSettings.UpdateSleepSettings();
                            return true;
                    }
                    return false;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Screen,
                .submenu = SettingSubmenu::None,
                .setting = Setting::SleepWhilePlayingMedia,
                .editable = true,
                .static_description = GetLanguageString("set_sleep_media_play"),
                .dyn_value_cb = []() {
                    return FormatBoolean(g_GlobalSettings.sleep_settings.flags & SleepFlag_SleepsWhilePlayingMedia);
                },
                .select_cb = []() {
                    if(g_GlobalSettings.sleep_settings.flags & SleepFlag_SleepsWhilePlayingMedia) {
                        g_GlobalSettings.sleep_settings.flags &= ~SleepFlag_SleepsWhilePlayingMedia;
                    }
                    else {
                        g_GlobalSettings.sleep_settings.flags |= SleepFlag_SleepsWhilePlayingMedia;
                    }
                    g_GlobalSettings.UpdateSleepSettings();
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Screen,
                .submenu = SettingSubmenu::None,
                .setting = Setting::SleepWakesAtPowerStateChange,
                .editable = true,
                .static_description = GetLanguageString("set_sleep_wake_power_state"),
                .dyn_value_cb = []() {
                    return FormatBoolean(g_GlobalSettings.sleep_settings.flags & SleepFlag_WakesAtPowerStateChange);
                },
                .select_cb = []() {
                    if(g_GlobalSettings.sleep_settings.flags & SleepFlag_WakesAtPowerStateChange) {
                        g_GlobalSettings.sleep_settings.flags &= ~SleepFlag_WakesAtPowerStateChange;
                    }
                    else {
                        g_GlobalSettings.sleep_settings.flags |= SleepFlag_WakesAtPowerStateChange;
                    }
                    g_GlobalSettings.UpdateSleepSettings();
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Dev,
                .submenu = SettingSubmenu::None,
                .setting = Setting::BatteryLot,
                .editable = false,
                .static_description = GetLanguageString("set_battery_lot"),
                .static_value = g_GlobalSettings.battery_lot.lot
            });

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConsoleFirmware,
                .editable = false,
                .static_description = GetLanguageString("set_console_fw"),
                .static_value = std::string(g_GlobalSettings.fw_version.display_version) + " (" + g_GlobalSettings.fw_version.display_title + ")"
            });

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::AtmosphereFirmware,
                .editable = false,
                .static_description = GetLanguageString("set_ams_fw"),
                .static_value = g_GlobalSettings.ams_version.Format()
            });

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::AtmosphereEmummc,
                .editable = false,
                .static_description = GetLanguageString("set_ams_emummc"),
                .static_value = FormatBoolean(g_GlobalSettings.ams_is_emummc)
            });

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConsoleNickname,
                .editable = true,
                .static_description = GetLanguageString("set_console_nickname"),
                .dyn_value_cb = []() {
                    return std::string(g_GlobalSettings.nickname.nickname);
                },
                .select_cb = []() {
                    SwkbdConfig swkbd;
                    if(R_SUCCEEDED(swkbdCreate(&swkbd, 0))) {
                        swkbdConfigSetGuideText(&swkbd, GetLanguageString("swkbd_console_nick_guide").c_str());
                        swkbdConfigSetInitialText(&swkbd, g_GlobalSettings.nickname.nickname);
                        swkbdConfigSetStringLenMax(&swkbd, 32);
                        SetSysDeviceNickName new_name = {};
                        auto rc = swkbdShow(&swkbd, new_name.nickname, sizeof(new_name.nickname));
                        swkbdClose(&swkbd);
                        if(R_SUCCEEDED(rc)) {
                            g_GlobalSettings.nickname = new_name;
                            UL_RC_ASSERT(setsysSetDeviceNickname(&g_GlobalSettings.nickname));
                            return true;
                        }
                    }

                    return false;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConsoleTimezone,
                .editable = false,
                .static_description = GetLanguageString("set_console_timezone"),
                .static_value = std::string(g_GlobalSettings.timezone.name)
            });

            g_Settings.push_back({
                .menu = SettingMenu::uLaunch,
                .submenu = SettingSubmenu::None,
                .setting = Setting::UsbScreenCaptureEnabled,
                .editable = true,
                .static_description = GetLanguageString("set_usb_screen_capture_enabled"),
                .dyn_value_cb = []() {
                    bool usb_capture_enabled;
                    UL_ASSERT_TRUE(g_GlobalSettings.config.GetEntry(cfg::ConfigEntryId::UsbScreenCaptureEnabled, usb_capture_enabled));
                    return FormatBoolean(usb_capture_enabled);
                },
                .select_cb = []() {
                    bool usb_capture_enabled;
                    UL_ASSERT_TRUE(g_GlobalSettings.config.GetEntry(cfg::ConfigEntryId::UsbScreenCaptureEnabled, usb_capture_enabled));
                    const auto opt = g_MenuApplication->DisplayDialog(GetLanguageString("set_usb_screen_capture_enabled"), GetLanguageString("set_usb_screen_capture_info") + "\n" + (usb_capture_enabled ? GetLanguageString("set_usb_screen_capture_disable_conf") : GetLanguageString("set_usb_screen_capture_enable_conf")), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                    if(opt == 0) {
                        UL_ASSERT_TRUE(g_GlobalSettings.config.SetEntry(cfg::ConfigEntryId::UsbScreenCaptureEnabled, !usb_capture_enabled));
                        g_MenuApplication->ShowNotification(GetLanguageString("set_changed_reboot"));
                        return true;
                    }

                    return false;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Network,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConnectionSsid,
                .editable = true,
                .static_description = GetLanguageString("set_wifi_name"),
                .dyn_value_cb = []() {
                    auto connected_wifi_name = GetLanguageString("set_wifi_none");
                    NifmNetworkProfileData prof_data = {};
                    if(R_SUCCEEDED(nifmGetCurrentNetworkProfile(&prof_data))) {
                        connected_wifi_name = prof_data.wireless_setting_data.ssid;
                    }
                    return connected_wifi_name;
                },
                .select_cb = []() {
                    // Close uMenu and launch the connections applet
                    ShowNetConnect();
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConsoleLanguage,
                .editable = true,
                .static_description = GetLanguageString("set_console_lang"),
                .dyn_value_cb = []() {
                    return os::LanguageNameList[static_cast<u32>(g_GlobalSettings.language)];
                },
                .select_cb = []() {
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
                            const auto lang_code = g_GlobalSettings.available_language_codes[opt];
                            const auto rc = setsysSetLanguageCode(lang_code);
                            g_MenuApplication->DisplayDialog(GetLanguageString("set_lang"), R_SUCCEEDED(rc) ? GetLanguageString("set_lang_select_ok") : GetLanguageString("set_lang_select_error") + ": " + util::FormatResultDisplay(rc), { GetLanguageString("ok") }, true);
                            if(R_SUCCEEDED(rc)) {
                                // No need to change our global settings
                                RebootSystem();
                                // Never reached anyway
                                return true;
                            }
                        }
                    }

                    return false;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Network,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConsoleInformationUploadEnabled,
                .editable = true,
                .static_description = GetLanguageString("set_console_info_upload"),
                .dyn_value_cb = []() {
                    return FormatBoolean(g_GlobalSettings.console_info_upload_enabled);
                },
                .select_cb = []() {
                    g_GlobalSettings.console_info_upload_enabled = !g_GlobalSettings.console_info_upload_enabled;
                    UL_RC_ASSERT(setsysSetConsoleInformationUploadFlag(g_GlobalSettings.console_info_upload_enabled));
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Network,
                .submenu = SettingSubmenu::None,
                .setting = Setting::AutomaticApplicationDownloadEnabled,
                .editable = true,
                .static_description = GetLanguageString("set_auto_app_download"),
                .dyn_value_cb = []() {
                    return FormatBoolean(g_GlobalSettings.auto_app_download_enabled);
                },
                .select_cb = []() {
                    g_GlobalSettings.auto_app_download_enabled = !g_GlobalSettings.auto_app_download_enabled;
                    UL_RC_ASSERT(setsysSetAutomaticApplicationDownloadFlag(g_GlobalSettings.auto_app_download_enabled));
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Network,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConsoleAutoUpdateEnabled,
                .editable = true,
                .static_description = GetLanguageString("set_auto_update"),
                .dyn_value_cb = []() {
                    return FormatBoolean(g_GlobalSettings.auto_update_enabled);
                },
                .select_cb = []() {
                    g_GlobalSettings.auto_update_enabled = !g_GlobalSettings.auto_update_enabled;
                    UL_RC_ASSERT(setsysSetAutoUpdateEnableFlag(g_GlobalSettings.auto_update_enabled));
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Network,
                .submenu = SettingSubmenu::None,
                .setting = Setting::WirelessLanEnabled,
                .editable = true,
                .static_description = GetLanguageString("set_wireless_lan"),
                .dyn_value_cb = []() {
                    return FormatBoolean(g_GlobalSettings.wireless_lan_enabled);
                },
                .select_cb = []() {
                    g_GlobalSettings.wireless_lan_enabled = !g_GlobalSettings.wireless_lan_enabled;
                    UL_RC_ASSERT(setsysSetWirelessLanEnableFlag(g_GlobalSettings.wireless_lan_enabled));
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Bluetooth,
                .submenu = SettingSubmenu::None,
                .setting = Setting::BluetoothEnabled,
                .editable = true,
                .static_description = GetLanguageString("set_bluetooth"),
                .dyn_value_cb = []() {
                    return FormatBoolean(g_GlobalSettings.bluetooth_enabled);
                },
                .select_cb = []() {
                    g_GlobalSettings.bluetooth_enabled = !g_GlobalSettings.bluetooth_enabled;
                    UL_RC_ASSERT(setsysSetBluetoothEnableFlag(g_GlobalSettings.bluetooth_enabled));
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::Usb30Enabled,
                .editable = true,
                .static_description = GetLanguageString("set_usb_30"),
                .dyn_value_cb = []() {
                    return FormatBoolean(g_GlobalSettings.usb30_enabled);
                },
                .select_cb = []() {
                    g_GlobalSettings.usb30_enabled = !g_GlobalSettings.usb30_enabled;
                    UL_RC_ASSERT(setsysSetUsb30EnableFlag(g_GlobalSettings.usb30_enabled));
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::System,
                .submenu = SettingSubmenu::None,
                .setting = Setting::NfcEnabled,
                .editable = true,
                .static_description = GetLanguageString("set_nfc"),
                .dyn_value_cb = []() {
                    return FormatBoolean(g_GlobalSettings.nfc_enabled);
                },
                .select_cb = []() {
                    g_GlobalSettings.nfc_enabled = !g_GlobalSettings.nfc_enabled;
                    UL_RC_ASSERT(setsysSetNfcEnableFlag(g_GlobalSettings.nfc_enabled));
                    return true;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Dev,
                .submenu = SettingSubmenu::None,
                .setting = Setting::ConsoleSerialNumber,
                .editable = false,
                .static_description = GetLanguageString("set_serial_no"),
                .static_value = std::string(g_GlobalSettings.serial_no.number)
            });

            g_Settings.push_back({
                .menu = SettingMenu::Network,
                .submenu = SettingSubmenu::None,
                .setting = Setting::MacAddress,
                .editable = false,
                .static_description = GetLanguageString("set_mac_addr"),
                .dyn_value_cb = []() {
                    net::WlanMacAddress mac_addr;
                    UL_RC_ASSERT(net::GetMacAddress(mac_addr));
                    return net::FormatMacAddress(mac_addr);
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::Network,
                .submenu = SettingSubmenu::None,
                .setting = Setting::IpAddress,
                .editable = false,
                .static_description = GetLanguageString("set_ip_addr"),
                .dyn_value_cb = []() {
                    return net::GetConsoleIpAddress();
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::uLaunch,
                .submenu = SettingSubmenu::None,
                .setting = Setting::uLaunchVersion,
                .editable = false,
                .static_description = GetLanguageString("set_ul_version"),
                .dyn_value_cb = []() {
                    return UL_VERSION;
                }
            });

            g_Settings.push_back({
                .menu = SettingMenu::uLaunch,
                .submenu = SettingSubmenu::None,
                .setting = Setting::AudioService,
                .editable = false,
                .static_description = GetLanguageString("set_audio_service"),
                .static_value = GetAudioService()
            });
        }

    }

    SettingsMenuLayout::SettingsMenuLayout() : IMenuLayout() {
        LoadSettings();
        this->cur_menu = static_cast<SettingMenu>(0);
        this->cur_submenu = SettingSubmenu::None;

        this->setting_edit_sfx = nullptr;
        this->setting_save_sfx = nullptr;
        this->back_sfx = nullptr;

        this->menu_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        this->menu_text->SetColor(g_MenuApplication->GetTextColor());
        g_GlobalSettings.ApplyConfigForElement("settings_menu", "menu_text", this->menu_text);
        this->Add(this->menu_text);

        this->submenu_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        this->submenu_text->SetColor(g_MenuApplication->GetTextColor());
        g_GlobalSettings.ApplyConfigForElement("settings_menu", "submenu_text", this->submenu_text);
        this->Add(this->submenu_text);

        this->settings_menu = pu::ui::elm::Menu::New(0, 0, SettingsMenuWidth, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), SettingsMenuItemSize, SettingsMenuItemsToShow);
        g_GlobalSettings.ApplyConfigForElement("settings_menu", "settings_menu", this->settings_menu);
        this->Add(this->settings_menu);

        this->input_bar = InputBar::New(0, 0, "ui/Settings/InputBarBackground");
        g_GlobalSettings.ApplyConfigForElement("settings_menu", "input_bar", this->input_bar);
        this->Add(this->input_bar);
        this->inputs_changed = true;
    }

    void SettingsMenuLayout::LoadSfx() {
        this->setting_edit_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Settings/SettingEdit.wav"));
        this->setting_save_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Settings/SettingSave.wav"));
        this->back_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Settings/Back.wav"));
        this->setting_menu_move_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Settings/SettingMenuMove.wav"));
    }

    void SettingsMenuLayout::DisposeSfx() {
        pu::audio::DestroySfx(this->setting_edit_sfx);
        pu::audio::DestroySfx(this->setting_save_sfx);
        pu::audio::DestroySfx(this->back_sfx);
        pu::audio::DestroySfx(this->setting_menu_move_sfx);
    }

    void SettingsMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        const auto can_move_left = static_cast<u32>(this->cur_menu) > 0;
        const auto can_move_right = (static_cast<u32>(this->cur_menu) + 1) < static_cast<u32>(SettingMenu::Count);
        const auto is_in_submenu = this->cur_submenu != SettingSubmenu::None;

        if(keys_down & HidNpadButton_B) {
            pu::audio::PlaySfx(this->back_sfx);

            if(is_in_submenu) {
                g_MenuApplication->SetBackgroundFade();
                g_MenuApplication->FadeOut();

                this->cur_submenu = SettingSubmenu::None;
                this->inputs_changed = true;
                this->Reload(false);

                g_MenuApplication->FadeIn();
            }
            else {
                g_MenuApplication->LoadMenu(MenuType::Main);
            }
        }
        else if(keys_down & HidNpadButton_L) {
            if(can_move_left) {
                pu::audio::PlaySfx(this->setting_menu_move_sfx);
                g_MenuApplication->SetBackgroundFade();
                g_MenuApplication->FadeOut();

                this->cur_menu = static_cast<SettingMenu>(static_cast<u32>(this->cur_menu) - 1);
                this->inputs_changed = true;
                this->Reload(false);

                g_MenuApplication->FadeIn();
            }
        }
        else if(keys_down & HidNpadButton_R) {
            if(can_move_right) {
                pu::audio::PlaySfx(this->setting_menu_move_sfx);
                g_MenuApplication->SetBackgroundFade();
                g_MenuApplication->FadeOut();

                this->cur_menu = static_cast<SettingMenu>(static_cast<u32>(this->cur_menu) + 1);
                this->inputs_changed = true;
                this->Reload(false);

                g_MenuApplication->FadeIn();
            }
        }

        //////////////////////////////

        if(this->inputs_changed) {
            this->inputs_changed = false;

            this->input_bar->ClearInputs();
            this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_settings_select"));
            if(can_move_left) {
                this->input_bar->AddSetInput(HidNpadButton_L, GetLanguageString("input_settings_left"));
            }
            if(can_move_right) {
                this->input_bar->AddSetInput(HidNpadButton_R, GetLanguageString("input_settings_right"));
            }
            this->input_bar->AddSetInput(HidNpadButton_B, GetLanguageString("input_settings_back"));
        }
    }

    bool SettingsMenuLayout::OnHomeButtonPress() {
        pu::audio::PlaySfx(this->back_sfx);

        g_MenuApplication->LoadMenu(MenuType::Main);
        return true;
    }

    void SettingsMenuLayout::Reload(const bool soft) {
        if(this->cur_submenu != SettingSubmenu::None) {
            /*
            this->submenu_text->SetVisible(true);
            this->submenu_text->SetText("");
            */
        }
        else {
            this->submenu_text->SetVisible(false);
        }
        this->menu_text->SetText(GetMenuName(this->cur_menu));

        if(soft) {
            for(auto &setting: g_Settings) {
                if((setting.menu == this->cur_menu) && (setting.submenu == this->cur_submenu)) {
                    setting.menu_item->SetName(setting.Format());
                }
            }
            this->settings_menu->ForceReloadItems();
        }
        else {
            this->settings_menu->ClearItems();
            for(auto &setting: g_Settings) {
                if((setting.menu == this->cur_menu) && (setting.submenu == this->cur_submenu)) {
                    setting.menu_item = pu::ui::elm::MenuItem::New(setting.Format());
                    if(setting.editable) {
                        setting.menu_item->AddOnKey([&]() {
                            pu::audio::PlaySfx(this->setting_edit_sfx);
                            const auto changed = setting.select_cb();
                            if(changed) {
                                pu::audio::PlaySfx(this->setting_save_sfx);
                                g_GlobalSettings.SaveConfig();
                                this->Reload(true);
                            }
                        });
                    }
                    setting.menu_item->SetIcon(setting.editable ? GetEditableSettingIconTexture() : GetNonEditableSettingIconTexture());
                    setting.menu_item->SetColor(g_MenuApplication->GetTextColor());
                    this->settings_menu->AddItem(setting.menu_item);
                }
            }
        }
    }

}
