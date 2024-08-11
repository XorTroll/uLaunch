
#pragma once
#include <ul/cfg/cfg_Config.hpp>
#include <ul/menu/menu_Entries.hpp>
#include <ul/os/os_System.hpp>
#include <pu/Plutonium>
#include <ul/menu/smi/smi_Commands.hpp>

namespace ul::menu::ui {

    std::string TryGetActiveThemeResource(const std::string &resource_base);

    std::string TryFindImage(const std::string &path_no_ext);
    pu::sdl2::Texture TryFindLoadImage(const std::string &path_no_ext);

    inline pu::sdl2::TextureHandle::Ref TryFindLoadImageHandle(const std::string &path_no_ext) {
        return pu::sdl2::TextureHandle::New(TryFindLoadImage(path_no_ext));
    }

    void LoadCommonTextures();
    pu::sdl2::TextureHandle::Ref GetBackgroundTexture();
    pu::sdl2::TextureHandle::Ref GetLogoTexture();

    pu::sdl2::TextureHandle::Ref GetEditableSettingIconTexture();
    pu::sdl2::TextureHandle::Ref GetNonEditableSettingIconTexture();

    void LoadSelectedUserIconTexture();
    pu::sdl2::TextureHandle::Ref GetSelectedUserIconTexture();

    struct GlobalSettings {
        SetSysFirmwareVersion fw_version;
        SetSysSerialNumber serial_no;
        SetSysSleepSettings sleep_settings;
        SetRegion region;
        SetSysPrimaryAlbumStorage album_storage;
        bool nfc_enabled;
        bool usb30_enabled;
        bool bluetooth_enabled;
        bool wireless_lan_enabled;
        bool auto_update_enabled;
        bool auto_app_download_enabled;
        bool console_info_upload_enabled;
        u64 available_language_codes[os::LanguageNameCount];
        SetLanguage language;
        SetSysDeviceNickName nickname;
        TimeLocationName timezone;
        SetBatteryLot battery_lot;

        ul::Version ams_version;
        bool ams_is_emummc;
        smi::SystemStatus system_status;
        std::string initial_last_menu_fs_path;
        u32 initial_last_menu_index;
        ul::cfg::Config config;
        ul::cfg::Theme active_theme;
        ul::util::JSON default_lang;
        ul::util::JSON main_lang;
        u64 cache_hb_takeover_app_id;

        std::vector<u64> added_app_ids;
        std::vector<u64> deleted_app_ids;
        std::vector<u64> in_verify_app_ids;

        inline bool IsTitleSuspended() {
            return this->system_status.suspended_app_id != 0;
        }

        inline bool IsHomebrewSuspended() {
            return strlen(this->system_status.suspended_hb_target_ipt.nro_path) > 0;
        }

        inline bool IsSuspended() {
            return this->IsTitleSuspended() || this->IsHomebrewSuspended();
        }

        inline void ResetSuspendedApplication() {
            // Blanking the whole status would also blank the selected user, thus we only blank the relevant params
            this->system_status.suspended_app_id = {};
            this->system_status.suspended_hb_target_ipt = {};
        }

        inline void UpdateMenuIndex(const u32 idx) {
            UL_RC_ASSERT(smi::UpdateMenuIndex(idx));
            this->system_status.last_menu_index = idx;
        }

        inline bool IsEntrySuspended(const Entry &entry) {
            if(entry.Is<EntryType::Application>()) {
                return entry.app_info.record.application_id == this->system_status.suspended_app_id;
            }
            else if(entry.Is<EntryType::Homebrew>()) {
                // Enough to compare the NRO path
                return memcmp(this->system_status.suspended_hb_target_ipt.nro_path, entry.hb_info.nro_target.nro_path, sizeof(entry.hb_info.nro_target.nro_path)) == 0;
            }

            return false;
        }

        inline void SaveConfig() {
            cfg::SaveConfig(this->config);
            UL_RC_ASSERT(smi::ReloadConfig());
        }

        inline void SetHomebrewTakeoverApplicationId(const u64 app_id) {
            this->cache_hb_takeover_app_id = app_id;

            UL_ASSERT_TRUE(this->config.SetEntry(cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, this->cache_hb_takeover_app_id));
            this->SaveConfig();
        }

        inline void SetSelectedUser(const AccountUid user_id) {
            this->system_status.selected_user = user_id;
            UL_RC_ASSERT(ul::menu::smi::SetSelectedUser(user_id));
            LoadSelectedUserIconTexture();
        }

        inline void SetActiveTheme(const ul::cfg::Theme &theme) {
            this->active_theme = theme;
            UL_ASSERT_TRUE(this->config.SetEntry(cfg::ConfigEntryId::ActiveThemeName, this->active_theme.name));
            this->SaveConfig();
        }

        inline void UpdateSleepSettings() {
            UL_RC_ASSERT(setsysSetSleepSettings(&this->sleep_settings));
        }
    };

    void RebootSystem();
    void ShutdownSystem();
    void SleepSystem();

    void ShowAboutDialog();

    void ShowSettingsMenu();
    void ShowThemesMenu();
    void ShowUserPage();
    void ShowControllerSupport();
    void ShowWebPage();
    void ShowAlbum();
    void ShowMiiEdit();
    void ShowNetConnect();
    void ShowCabinet();

    void ShowPowerDialog();

}
