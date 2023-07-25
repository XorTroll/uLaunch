
#pragma once
#include <ul/menu/ui/ui_TransitionGuard.hpp>
#include <ul/menu/ui/ui_StartupLayout.hpp>
#include <ul/menu/ui/ui_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_ThemeMenuLayout.hpp>
#include <ul/menu/ui/ui_SettingsMenuLayout.hpp>
#include <ul/menu/ui/ui_LanguagesMenuLayout.hpp>
#include <ul/smi/smi_Protocol.hpp>

namespace ul::menu::ui {

    std::string GetLanguageString(const std::string &name);

    enum class MenuType {
        Startup,
        Main,
        Settings,
        Theme,
        Languages
    };

    void OnMessage(const smi::MenuMessageContext msg_ctx);

    class MenuApplication : public pu::ui::Application {
        private:
            smi::MenuStartMode start_mode;
            StartupLayout::Ref startup_lyt;
            MainMenuLayout::Ref main_menu_lyt;
            ThemeMenuLayout::Ref theme_menu_lyt;
            SettingsMenuLayout::Ref settings_menu_lyt;
            LanguagesMenuLayout::Ref languages_menu_lyt;
            pu::ui::extras::Toast::Ref notif_toast;
            smi::SystemStatus system_status;
            bool launch_failed;
            char chosen_hb[FS_MAX_PATH];
            MenuType loaded_menu;
            util::JSON ui_json;
            util::JSON bgm_json;
            bool bgm_loop;
            u32 bgm_fade_in_ms;
            u32 bgm_fade_out_ms;
            pu::audio::Music bgm;
            pu::ui::Color text_clr;
            pu::ui::Color menu_focus_clr;
            pu::ui::Color menu_bg_clr;

        public:
            using Application::Application;

            ~MenuApplication() {
                pu::audio::DestroyMusic(this->bgm);
            }
            
            PU_SMART_CTOR(MenuApplication)

            void OnLoad() override;

            inline void Initialize(const smi::MenuStartMode start_mode, const smi::SystemStatus system_status, const util::JSON ui_json) {
                this->start_mode = start_mode;
                this->system_status = system_status;
                this->ui_json = ui_json;
            }

            inline void LoadMainMenu() {
                this->main_menu_lyt->SetUser(this->system_status.selected_user);
                this->LoadLayout(this->main_menu_lyt);
                this->loaded_menu = MenuType::Main;
            }

            inline void LoadStartupMenu() {
                this->StopPlayBGM();
                this->startup_lyt->ReloadMenu();
                this->LoadLayout(this->startup_lyt);
                this->loaded_menu = MenuType::Startup;
            }

            inline void LoadThemeMenu() {
                this->theme_menu_lyt->Reload();
                this->LoadLayout(this->theme_menu_lyt);
                this->loaded_menu = MenuType::Theme;
            }

            inline void LoadSettingsMenu() {
                this->settings_menu_lyt->Reload(true);
                this->LoadLayout(this->settings_menu_lyt);
                this->loaded_menu = MenuType::Settings;
            }

            inline void LoadSettingsLanguagesMenu() {
                this->languages_menu_lyt->Reload();
                this->LoadLayout(this->languages_menu_lyt);
                this->loaded_menu = MenuType::Languages;
            }

            inline bool IsTitleSuspended() {
                return this->system_status.suspended_app_id != 0;
            }

            inline bool IsHomebrewSuspended() {
                return strlen(this->system_status.suspended_hb_target_ipt.nro_path) > 0;
            }

            inline bool IsSuspended() {
                return this->IsTitleSuspended() || this->IsHomebrewSuspended();
            }
            
            inline bool EqualsSuspendedHomebrewPath(const std::string &path) {
                return this->system_status.suspended_hb_target_ipt.nro_path == path;
            }

            inline smi::SystemStatus &GetStatus() {
                return this->system_status;
            }

            inline void ResetSuspendedApplication() {
                // Blanking the whole status would also blank the selected user, thus we only blank the params
                this->system_status.suspended_app_id = {};
                this->system_status.suspended_hb_target_ipt = {};
            }

            inline bool GetLaunchFailed() {
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

            inline std::string GetChosenHomebrew() {
                const std::string res = this->chosen_hb;
                memset(this->chosen_hb, 0, sizeof(this->chosen_hb));
                return res;
            }

            inline void NotifyHomebrewChosen(const char (&chosen_hb_path)[FS_MAX_PATH]) {
                util::CopyToStringBuffer(this->chosen_hb, chosen_hb_path);
            }

            void ShowNotification(const std::string &text, const u64 timeout = 1500);

            template<typename T>
            inline T GetUIConfigValue(const std::string &name, const T def) {
                return this->ui_json.value<T>(name, def);
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

                        if(set_coords) {
                            if(elem_json.count("x")) {
                                const s32 x = elem_json.value("x", 0);
                                elem->SetX(x);
                            }
                            if(elem_json.count("y")) {
                                const s32 y = elem_json.value("y", 0);
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

            void StartPlayBGM();
            void StopPlayBGM();

            inline StartupLayout::Ref &GetStartupLayout() {
                return this->startup_lyt;
            }

            inline MainMenuLayout::Ref &GetMainMenuLayout() {
                return this->main_menu_lyt;
            }

            inline ThemeMenuLayout::Ref &GetThemeMenuLayout() {
                return this->theme_menu_lyt;
            }

            inline SettingsMenuLayout::Ref &GetSettingsMenuLayout() {
                return this->settings_menu_lyt;
            }
            
            inline LanguagesMenuLayout::Ref &GetLanguagesMenuLayout() {
                return this->languages_menu_lyt;
            }

            void SetSelectedUser(const AccountUid user_id);
            
            inline AccountUid GetSelectedUser() {
                return this->system_status.selected_user;
            }

            inline MenuType GetCurrentLoadedMenu() {
                return this->loaded_menu;
            }
    };

    inline void RegisterOnMessageCallback() {
        smi::RegisterOnMessageDetect(&OnMessage);
    }

}