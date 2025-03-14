
#pragma once
#include <ul/menu/ui/ui_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_StartupMenuLayout.hpp>
#include <ul/menu/ui/ui_ThemesMenuLayout.hpp>
#include <ul/menu/ui/ui_SettingsMenuLayout.hpp>
#include <ul/menu/ui/ui_LockscreenMenuLayout.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

namespace ul::menu::ui {

    std::string GetLanguageString(const std::string &name);

    enum class MenuType {
        Main,
        Startup,
        Themes,
        Settings,
        Lockscreen
    };
    
    struct MenuBgmEntry {
        bool bgm_loop;
        u32 bgm_fade_in_ms;
        u32 bgm_fade_out_ms;
        pu::audio::Music bgm;
    };

    void OnMessage(const smi::MenuMessageContext &msg_ctx);

    class MenuApplication : public pu::ui::Application {
        public:
            static constexpr bool DefaultBgmLoop = true;
            static constexpr u32 DefaultBgmFadeInMs = 1500;
            static constexpr u32 DefaultBgmFadeOutMs = 500;
            using MenuFadeCallback = std::function<void()>;

            static constexpr u8 DefaultFadeAlphaIncrementSteps = 20;
            static constexpr u8 FastFadeAlphaIncrementSteps = 12;

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
            LockscreenMenuLayout::Ref lockscreen_menu_lyt;
            MenuBgmEntry lockscreen_menu_bgm;
            pu::ui::extras::Toast::Ref notif_toast;
            bool launch_failed;
            Result pending_gc_mount_rc;
            bool needs_app_records_reload;
            bool needs_app_entries_reload;
            char chosen_hb[FS_MAX_PATH];
            u64 verify_finished_app_id;
            Result verify_rc;
            Result verify_detail_rc;
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
                    case MenuType::Lockscreen:
                        return this->lockscreen_menu_bgm;
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

            inline void Initialize(const smi::MenuStartMode start_mode) {
                this->start_mode = start_mode;
            }

            inline void Finalize() {
                this->StopPlayBgm();
                this->ResetFade();
                this->DisposeAllAudio();
                this->CloseWithFadeOut(true);
            }

            void SetBackgroundFade();

            inline void ResetFade() {
                this->ResetFadeBackgroundImage();
            }

            void LoadMenuByType(const MenuType type, const bool fade = true, MenuFadeCallback fade_cb = nullptr);

            inline void NotifyLaunchFailed() {
                this->launch_failed = true;
            }

            inline bool GetConsumeLastLaunchFailed() {
                const auto res = this->launch_failed;
                this->launch_failed = 0;
                return res;
            }

            inline void NotifyHomebrewChosen(const char (&chosen_hb_path)[FS_MAX_PATH]) {
                util::CopyToStringBuffer(this->chosen_hb, chosen_hb_path);
            }

            inline bool HasChosenHomebrew() {
                return this->chosen_hb[0] != '\0';
            }

            inline std::string GetConsumeChosenHomebrew() {
                const std::string res = this->chosen_hb;
                memset(this->chosen_hb, 0, sizeof(this->chosen_hb));
                return res;
            }

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

            inline void NotifyApplicationRecordReloadNeeded() {
                this->needs_app_records_reload = true;
            }

            inline bool GetConsumeApplicationRecordReloadNeeded() {
                const auto needs_reload = this->needs_app_records_reload;
                this->needs_app_records_reload = false;
                return needs_reload;
            }

            inline void NotifyApplicationEntryReloadNeeded() {
                this->needs_app_entries_reload = true;
            }

            inline bool GetConsumeApplicationEntryReloadNeeded() {
                const auto needs_reload = this->needs_app_entries_reload;
                this->needs_app_entries_reload = false;
                return needs_reload;
            }

            inline void NotifyVerifyFinished(const u64 app_id, const Result rc, const Result detail_rc) {
                this->verify_finished_app_id = app_id;
                this->verify_rc = rc;
                this->verify_detail_rc = detail_rc;
            }

            inline bool HasVerifyFinishedPending() {
                return this->verify_finished_app_id != 0;
            }

            inline u64 GetConsumeVerifyFinishedApplicationId() {
                const auto app_id = this->verify_finished_app_id;
                this->verify_finished_app_id = 0;
                return app_id;
            }

            inline Result GetConsumeVerifyResult() {
                const auto rc = this->verify_rc;
                this->verify_rc = ResultSuccess;
                return rc;
            }

            inline Result GetConsumeVerifyDetailResult() {
                const auto rc = this->verify_detail_rc;
                this->verify_detail_rc = ResultSuccess;
                return rc;
            }

            inline void NotifyApplicationVerifyProgress(const u64 app_id, const float progress = NAN) {
                if(this->loaded_menu == MenuType::Main) {
                    this->GetLayout<MainMenuLayout>()->UpdateApplicationVerifyProgress(app_id, progress);
                }
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

            int DisplayDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool use_last_opt_as_cancel, pu::sdl2::TextureHandle::Ref icon = {});
    };

    inline void RegisterMenuOnMessageDetect() {
        smi::RegisterOnMessageDetect(&OnMessage);
    }

}
