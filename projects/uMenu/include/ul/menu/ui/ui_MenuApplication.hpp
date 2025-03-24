
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

    void OnMessage(const smi::MenuMessageContext &msg_ctx);

    class MenuApplication : public pu::ui::Application {
        public:
            using MenuFadeCallback = std::function<void()>;

            static constexpr u8 DefaultFadeAlphaIncrementSteps = 20;
            static constexpr u8 FastFadeAlphaIncrementSteps = 12;

        private:
            smi::MenuStartMode start_mode;
            MainMenuLayout::Ref main_menu_lyt;
            StartupMenuLayout::Ref startup_menu_lyt;
            ThemesMenuLayout::Ref themes_menu_lyt;
            SettingsMenuLayout::Ref settings_menu_lyt;
            LockscreenMenuLayout::Ref lockscreen_menu_lyt;
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
            pu::ui::Color text_clr;
            pu::ui::Color menu_focus_clr;
            pu::ui::Color menu_bg_clr;
            pu::ui::Color dialog_title_clr;
            pu::ui::Color dialog_cnt_clr;
            pu::ui::Color dialog_opt_clr;
            pu::ui::Color dialog_clr;
            pu::ui::Color dialog_over_clr;
            u8 *screen_capture_buf;

            MenuBgmEntry &GetCurrentMenuBgm();

            void EnsureLayoutCreated(const MenuType type);

        public:
            using Application::Application;
            PU_SMART_CTOR(MenuApplication)

            void OnLoad() override;

            inline void Initialize(const smi::MenuStartMode start_mode) {
                this->start_mode = start_mode;
            }

            void Finalize();

            void SetBackgroundFade();

            inline void ResetFade() {
                this->ResetFadeBackgroundImage();
            }

            void LoadMenu(const MenuType type, const bool fade = true, MenuFadeCallback fade_cb = nullptr);

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

            void LoadBgmSfxForCreatedMenus();
            void DisposeAllSfx();

            int DisplayDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool use_last_opt_as_cancel, pu::sdl2::TextureHandle::Ref icon = {});
    };

    inline void RegisterMenuOnMessageDetect() {
        smi::RegisterOnMessageDetect(&OnMessage);
    }

}
