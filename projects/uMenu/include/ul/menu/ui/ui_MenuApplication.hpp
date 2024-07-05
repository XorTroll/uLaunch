
#pragma once
#include <ul/menu/ui/ui_StartupMenuLayout.hpp>
#include <ul/menu/ui/ui_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_ThemesMenuLayout.hpp>
#include <ul/menu/ui/ui_SettingsMenuLayout.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

namespace ul::menu::ui {

    std::string GetLanguageString(const std::string &name);

    enum class MenuType {
        Main,
        Startup,
        Themes,
        Settings
    };
    
    struct MenuBgmEntry {
        bool bgm_loop;
        u32 bgm_fade_in_ms;
        u32 bgm_fade_out_ms;
        pu::audio::Music bgm;
    };

    void OnMessage(const smi::MenuMessageContext msg_ctx);

    class MenuApplication : public pu::ui::Application {
        public:
            static constexpr bool DefaultBgmLoop = true;
            static constexpr u32 DefaultBgmFadeInMs = 1500;
            static constexpr u32 DefaultBgmFadeOutMs = 500;
            using MenuFadeCallback = std::function<void()>;

        private:
            smi::MenuStartMode start_mode;
            MainMenuLayout::Ref main_menu_lyt;
            MenuBgmEntry main_menu_bgm;
            StartupMenuLayout::Ref startup_menu_lyt;
            MenuBgmEntry startup_menu_bgm;
            ThemesMenuLayout::Ref themes_menu_lyt;
            MenuBgmEntry themes_menu_bgm;
            SettingsMenuLayout::Ref settings_menu_lyt;
            MenuBgmEntry settings_menu_bgm;
            pu::ui::extras::Toast::Ref notif_toast;
            smi::SystemStatus system_status;
            bool launch_failed;
            Result pending_gc_mount_rc;
            char chosen_hb[FS_MAX_PATH];
            u64 takeover_app_id;
            MenuType loaded_menu;
            util::JSON ui_json;
            util::JSON bgm_json;
            pu::ui::Color text_clr;
            pu::ui::Color menu_focus_clr;
            pu::ui::Color menu_bg_clr;
            pu::ui::Color dialog_title_clr;
            pu::ui::Color dialog_cnt_clr;
            pu::ui::Color dialog_opt_clr;
            pu::ui::Color dialog_clr;
            pu::ui::Color dialog_over_clr;

            inline MenuBgmEntry &GetCurrentMenuBgm() {
                switch(this->loaded_menu) {
                    case MenuType::Main:
                        return this->main_menu_bgm;
                    case MenuType::Startup:
                        return this->startup_menu_bgm;
                    case MenuType::Themes:
                        return this->themes_menu_bgm;
                    case MenuType::Settings:
                        return this->settings_menu_bgm;
                }

                UL_ASSERT_TRUE(false && "Invalid current menu?");
            }

        public:
            using Application::Application;

            ~MenuApplication() {
                pu::audio::DestroyMusic(this->main_menu_bgm.bgm);
                pu::audio::DestroyMusic(this->startup_menu_bgm.bgm);
                pu::audio::DestroyMusic(this->themes_menu_bgm.bgm);
                pu::audio::DestroyMusic(this->settings_menu_bgm.bgm);
            }
            
            PU_SMART_CTOR(MenuApplication)

            void OnLoad() override;

            inline void Initialize(const smi::MenuStartMode start_mode, const smi::SystemStatus system_status, const util::JSON ui_json) {
                this->start_mode = start_mode;
                this->system_status = system_status;
                this->ui_json = ui_json;
            }

            inline void Finalize() {
                this->StopPlayBgm();
                this->ResetFade();
                this->DisposeAllAudio();
                this->CloseWithFadeOut();
            }

            void SetBackgroundFade();

            inline void ResetFade() {
                this->ResetFadeBackgroundImage();
            }

            void LoadMenuByType(const MenuType type, const bool fade = true, MenuFadeCallback fade_cb = nullptr);

            inline bool IsTitleSuspended() {
                return this->system_status.suspended_app_id != 0;
            }

            inline bool IsHomebrewSuspended() {
                return strlen(this->system_status.suspended_hb_target_ipt.nro_path) > 0;
            }

            inline bool IsSuspended() {
                return this->IsTitleSuspended() || this->IsHomebrewSuspended();
            }

            inline smi::SystemStatus &GetStatus() {
                return this->system_status;
            }

            inline void ResetSuspendedApplication() {
                // Blanking the whole status would also blank the selected user, thus we only blank the params
                this->system_status.suspended_app_id = {};
                this->system_status.suspended_hb_target_ipt = {};
            }

            inline bool GetConsumeLastLaunchFailed() {
                const auto res = this->launch_failed;
                this->launch_failed = 0;
                return res;
            }

            inline void NotifyLaunchFailed() {
                this->launch_failed = true;
            }

            inline bool HasChosenHomebrew() {
                return this->chosen_hb[0] != '\0';
            }

            inline std::string GetConsumeChosenHomebrew() {
                const std::string res = this->chosen_hb;
                memset(this->chosen_hb, 0, sizeof(this->chosen_hb));
                return res;
            }

            inline void NotifyHomebrewChosen(const char (&chosen_hb_path)[FS_MAX_PATH]) {
                util::CopyToStringBuffer(this->chosen_hb, chosen_hb_path);
            }

            inline u64 GetTakeoverApplicationId() {
                return this->takeover_app_id;
            }

            void SetTakeoverApplicationId(const u64 app_id);

            inline void NotifyGameCardMountFailure(const Result rc) {
                this->pending_gc_mount_rc = rc;
            }

            inline bool HasGameCardMountFailure() {
                return R_FAILED(this->pending_gc_mount_rc);
            }

            Result GetConsumeGameCardMountFailure() {
                const auto gc_rc = this->pending_gc_mount_rc;
                this->pending_gc_mount_rc = ResultSuccess;
                return gc_rc;
            }

            inline void UpdateMenuIndex(const u32 idx) {
                UL_RC_ASSERT(smi::UpdateMenuIndex(idx));
                this->system_status.last_menu_index = idx;
            }

            void ShowNotification(const std::string &text, const u64 timeout = 1500);

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
                if(align == "up") {
                    out_align = pu::ui::elm::VerticalAlign::Up;
                    return true;
                }
                if(align == "center") {
                    out_align = pu::ui::elm::VerticalAlign::Center;
                    return true;
                }
                if(align == "down") {
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

            template<typename Elem>
            inline void ApplyConfigForElement(const std::string &menu, const std::string &name, std::shared_ptr<Elem> &elem, const bool apply_visible = true) {
                if(this->ui_json.count(menu)) {
                    const auto menu_json = this->ui_json[menu];
                    if(menu_json.count(name)) {
                        const auto elem_json = menu_json[name];

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
            }

            inline pu::ui::Color GetTextColor() {
                return this->text_clr;
            }

            inline pu::ui::Color GetMenuFocusColor() {
                return this->menu_focus_clr;
            }

            inline pu::ui::Color GetMenuBackgroundColor() {
                return this->menu_bg_clr;
            }

            void StartPlayBgm();
            void StopPlayBgm();

            inline void DisposeAllAudio() {
                this->main_menu_lyt->DisposeAudio();
                this->startup_menu_lyt->DisposeAudio();
                this->themes_menu_lyt->DisposeAudio();
                this->settings_menu_lyt->DisposeAudio();
            }

            void SetSelectedUser(const AccountUid user_id);

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

            inline bool IsEntryHomebrewTakeoverApplication(const Entry &entry) {
                return entry.Is<EntryType::Application>() && (entry.app_info.record.application_id == this->takeover_app_id);
            }
            
            inline AccountUid GetSelectedUser() {
                return this->system_status.selected_user;
            }

            int DisplayDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool use_last_opt_as_cancel, pu::sdl2::TextureHandle::Ref icon = {});
    };

    inline void RegisterMenuOnMessageDetect() {
        smi::RegisterOnMessageDetect(&OnMessage);
    }

}
