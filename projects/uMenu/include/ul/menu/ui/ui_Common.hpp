
#pragma once
#include <ul/cfg/cfg_Config.hpp>
#include <ul/menu/menu_Entries.hpp>
#include <ul/os/os_System.hpp>
#include <pu/Plutonium>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/menu/ui/ui_MultiTextBlock.hpp>
#include <ul/menu/bt/bt_Manager.hpp>

namespace ul::menu::ui {

    std::string TryGetActiveThemeResource(const std::string &resource_base);

    std::string TryFindImage(const std::string &path_no_ext);
    pu::sdl2::Texture TryFindLoadImage(const std::string &path_no_ext);

    inline pu::sdl2::TextureHandle::Ref TryFindLoadImageHandle(const std::string &path_no_ext) {
        return pu::sdl2::TextureHandle::New(TryFindLoadImage(path_no_ext));
    }

    void InitializeResources();
    void DisposeAllBgm();

    pu::sdl2::TextureHandle::Ref GetBackgroundTexture();
    pu::sdl2::TextureHandle::Ref GetLogoTexture();

    pu::sdl2::TextureHandle::Ref GetEditableSettingIconTexture();
    pu::sdl2::TextureHandle::Ref GetNonEditableSettingIconTexture();

    void LoadSelectedUserIconTexture();
    pu::sdl2::TextureHandle::Ref GetSelectedUserIconTexture();

    bool TryGetUiElement(const std::string &menu, const std::string &elem, util::JSON &out_json);

    inline bool ParseHorizontalAlign(const std::string &align, pu::ui::elm::HorizontalAlign &out_align) {
        if(align == "left") {
            out_align = pu::ui::elm::HorizontalAlign::Left;
            return true;
        }
        if(align == "center") {
            out_align = pu::ui::elm::HorizontalAlign::Center;
            return true;
        }
        if(align == "right") {
            out_align = pu::ui::elm::HorizontalAlign::Right;
            return true;
        }

        return false;
    }

    inline bool ParseVerticalAlign(const std::string &align, pu::ui::elm::VerticalAlign &out_align) {
        if((align == "up") || (align == "top")) {
            out_align = pu::ui::elm::VerticalAlign::Up;
            return true;
        }
        if(align == "center") {
            out_align = pu::ui::elm::VerticalAlign::Center;
            return true;
        }
        if((align == "down") || (align == "bottom")) {
            out_align = pu::ui::elm::VerticalAlign::Down;
            return true;
        }

        return false;
    }

    inline bool ParseDefaultFontSize(const std::string &size, pu::ui::DefaultFontSize &out_size) {
        if(size == "small") {
            out_size = pu::ui::DefaultFontSize::Small;
            return true;
        }
        if(size == "medium") {
            out_size = pu::ui::DefaultFontSize::Medium;
            return true;
        }
        if(size == "medium-large") {
            out_size = pu::ui::DefaultFontSize::MediumLarge;
            return true;
        }
        if(size == "large") {
            out_size = pu::ui::DefaultFontSize::Large;
            return true;
        }

        return false;
    }
    
    // These UI values are required, we will assert otherwise (thus the error will be visible on our logs)

    util::JSON GetRequiredUiJsonValue(const std::string &name);

    template<typename T>
    inline T GetRequiredUiValue(const std::string &name) {
        return GetRequiredUiJsonValue(name).get<T>();
    }

    template<>
    inline pu::ui::Color GetRequiredUiValue(const std::string &name) {
        const auto clr_str = GetRequiredUiValue<std::string>(name);
        return pu::ui::Color::FromHex(clr_str);
    }

    struct MenuBgmEntry {
        static constexpr bool DefaultBgmLoop = true;
        static constexpr u32 DefaultBgmFadeInMs = 1500;
        static constexpr u32 DefaultBgmFadeOutMs = 500;

        bool bgm_loop;
        u32 bgm_fade_in_ms;
        u32 bgm_fade_out_ms;
        pu::audio::Music bgm;
    };

    bool TryGetBgmJsonValue(const std::string &name, util::JSON &out_json);

    void TryParseBgmEntry(const std::string &menu, const std::string &menu_bgm, MenuBgmEntry &out_entry);

    template<typename T>
    inline bool TryGetBgmValue(const std::string &name, T &out_value) {
        util::JSON json;
        if(TryGetBgmJsonValue(name, json)) {
            out_value = json.get<T>();
            return true;
        }

        return false;
    }

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

        MenuBgmEntry main_menu_bgm;
        MenuBgmEntry startup_menu_bgm;
        MenuBgmEntry themes_menu_bgm;
        MenuBgmEntry settings_menu_bgm;
        MenuBgmEntry lockscreen_menu_bgm;

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

        inline void ResetHomebrewTakeoverApplicationId() {
            this->SetHomebrewTakeoverApplicationId(os::InvalidApplicationId);
        }

        void InitializeEntries();
        void SetSelectedUser(const AccountUid user_id);

        inline void SetActiveTheme(const ul::cfg::Theme &theme) {
            this->active_theme = theme;
            UL_ASSERT_TRUE(this->config.SetEntry(cfg::ConfigEntryId::ActiveThemeName, this->active_theme.name));
            this->SaveConfig();
        }

        inline void UpdateSleepSettings() {
            UL_RC_ASSERT(setsysSetSleepSettings(&this->sleep_settings));
        }

        template<typename Elem>
        inline void ApplyConfigForElement(const std::string &menu, const std::string &name, std::shared_ptr<Elem> &elem, const bool apply_visible = true) {
            util::JSON elem_json;
            if(TryGetUiElement(menu, name, elem_json)) {
                auto set_coords = false;
                if(apply_visible) {
                    const auto visible = elem_json.value("visible", true);
                    elem->SetVisible(visible);
                    set_coords = visible;
                }
                else {
                    set_coords = true;
                }

                const auto h_align_str = elem_json.value("h_align", "");
                pu::ui::elm::HorizontalAlign h_align;
                if(ParseHorizontalAlign(h_align_str, h_align)) {
                    elem->SetHorizontalAlign(h_align);
                }

                const auto v_align_str = elem_json.value("v_align", "");
                pu::ui::elm::VerticalAlign v_align;
                if(ParseVerticalAlign(v_align_str, v_align)) {
                    elem->SetVerticalAlign(v_align);
                }

                if constexpr(std::is_same_v<Elem, pu::ui::elm::TextBlock>) {
                    const auto size_str = elem_json.value("font_size", "");
                    pu::ui::DefaultFontSize def_size;
                    if(ParseDefaultFontSize(size_str, def_size)) {
                        elem->SetFont(pu::ui::GetDefaultFont(def_size));
                    }

                    if(elem_json.count("clamp_width")) {
                        const s32 clamp_width = elem_json["clamp_width"];
                        elem->SetClampWidth(clamp_width);
                    }
                    if(elem_json.count("clamp_speed")) {
                        const s32 clamp_speed = elem_json["clamp_speed"];
                        elem->SetClampSpeed(clamp_speed);
                    }
                    if(elem_json.count("clamp_delay")) {
                        const s32 clamp_delay = elem_json["clamp_delay"];
                        elem->SetClampDelay(clamp_delay);
                    }
                }

                if constexpr(std::is_same_v<Elem, MultiTextBlock>) {
                    auto &texts = elem->GetAll();
                    const auto size_str = elem_json.value("font_size", "");
                    pu::ui::DefaultFontSize def_size;
                    if(ParseDefaultFontSize(size_str, def_size)) {
                        for(auto &tb: texts) {
                            tb->SetFont(pu::ui::GetDefaultFont(def_size));
                        }
                    }

                    if(elem_json.count("clamp_width")) {
                        const s32 clamp_width = elem_json["clamp_width"];
                        for(auto &tb: texts) {
                            tb->SetClampWidth(clamp_width);
                        }
                    }
                    if(elem_json.count("clamp_speed")) {
                        const s32 clamp_speed = elem_json["clamp_speed"];
                        for(auto &tb: texts) {
                            tb->SetClampSpeed(clamp_speed);
                        }
                    }
                    if(elem_json.count("clamp_delay")) {
                        const s32 clamp_delay = elem_json["clamp_delay"];
                        for(auto &tb: texts) {
                            tb->SetClampDelay(clamp_delay);
                        }
                    }
                }

                if(set_coords) {
                    if(elem_json.count("x")) {
                        const s32 x = elem_json["x"];
                        elem->SetX(x);
                    }
                    if(elem_json.count("y")) {
                        const s32 y = elem_json["y"];
                        elem->SetY(y);
                    }
                }
            }
        }
    };

    pu::sdl2::TextureHandle::Ref LoadApplicationIconTexture(const u64 app_id);

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

    size_t GetUsedHeapSize();

}
