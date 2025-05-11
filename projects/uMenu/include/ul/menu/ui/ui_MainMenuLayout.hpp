
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/menu/ui/ui_RawRgbaImage.hpp>
#include <ul/menu/ui/ui_ClickableImage.hpp>
#include <ul/menu/ui/ui_BackgroundScreenCapture.hpp>
#include <ul/menu/ui/ui_QuickMenu.hpp>
#include <ul/menu/ui/ui_InputBar.hpp>
#include <ul/menu/ui/ui_EntryMenu.hpp>
#include <ul/menu/ui/ui_Common.hpp>
#include <ul/menu/menu_Entries.hpp>
#include <ul/cfg/cfg_Config.hpp>

namespace ul::menu::ui {

    class MainMenuLayout : public IMenuLayout {
        public:
            // TODO (new): config in theme?
            static constexpr s64 MessagesWaitTimeSeconds = 1;
            static constexpr s64 TimeDotsDisplayChangeWaitTimeSeconds = 1;
            static constexpr u32 LogoSize = 90;

        private:
            bool last_quick_menu_on;
            pu::ui::elm::Image::Ref top_menu_default_bg;
            pu::ui::elm::Image::Ref top_menu_folder_bg;
            pu::ui::elm::Image::Ref top_menu_app_bg;
            pu::ui::elm::Image::Ref top_menu_hb_bg;
            pu::ui::elm::Image::Ref connection_top_icon;
            ClickableImage::Ref logo_top_icon;
            MultiTextBlock::Ref time_mtext;
            pu::ui::elm::TextBlock::Ref date_text;
            pu::ui::elm::TextBlock::Ref battery_text;
            pu::ui::elm::Image::Ref battery_top_icon;
            pu::ui::elm::Image::Ref battery_charging_top_icon;
            EntryMenu::Ref entry_menu;
            pu::ui::elm::Image::Ref entry_menu_bg;
            pu::ui::elm::Image::Ref entry_menu_left_icon;
            pu::ui::elm::Image::Ref entry_menu_right_icon;
            std::string cur_folder_path;
            pu::ui::elm::TextBlock::Ref cur_path_text;
            pu::ui::elm::TextBlock::Ref cur_entry_main_text;
            pu::ui::elm::TextBlock::Ref cur_entry_sub_text;
            QuickMenu::Ref quick_menu;
            InputBar::Ref input_bar;
            bool input_bar_changed;
            std::chrono::steady_clock::time_point startup_tp;
            bool start_time_elapsed;
            bool is_incrementing_decrementing;
            pu::audio::Sfx post_suspend_sfx;
            pu::audio::Sfx cursor_move_sfx;
            pu::audio::Sfx page_move_sfx;
            pu::audio::Sfx entry_select_sfx;
            pu::audio::Sfx entry_move_sfx;
            pu::audio::Sfx entry_swap_sfx;
            pu::audio::Sfx entry_cancel_select_sfx;
            pu::audio::Sfx entry_move_into_sfx;
            pu::audio::Sfx home_press_sfx;
            pu::audio::Sfx logoff_sfx;
            pu::audio::Sfx launch_app_sfx;
            pu::audio::Sfx launch_hb_sfx;
            pu::audio::Sfx close_suspended_sfx;
            pu::audio::Sfx open_folder_sfx;
            pu::audio::Sfx close_folder_sfx;
            pu::audio::Sfx open_mii_edit_sfx;
            pu::audio::Sfx open_web_browser_sfx;
            pu::audio::Sfx open_user_page_sfx;
            pu::audio::Sfx open_settings_sfx;
            pu::audio::Sfx open_themes_sfx;
            pu::audio::Sfx open_controllers_sfx;
            pu::audio::Sfx open_album_sfx;
            pu::audio::Sfx open_amiibo_sfx;
            pu::audio::Sfx open_quick_menu_sfx;
            pu::audio::Sfx close_quick_menu_sfx;
            pu::audio::Sfx resume_app_sfx;
            pu::audio::Sfx create_folder_sfx;
            pu::audio::Sfx create_hb_entry_sfx;
            pu::audio::Sfx entry_remove_sfx;
            pu::audio::Sfx error_sfx;
            pu::audio::Sfx menu_increment_sfx;
            pu::audio::Sfx menu_decrement_sfx;
            bool next_reload_user_changed;

            void DoMoveTo(const std::string &new_path);
            void menu_EntryInputPressed(const u64 keys_down);
            void menu_FocusedEntryChanged(const bool has_prev_entry, const bool is_prev_entry_suspended, const bool is_cur_entry_suspended);

            inline void PushFolder(const std::string &name) {
                this->cur_folder_path = fs::JoinPath(this->cur_folder_path, name);
                pu::audio::PlaySfx(this->open_folder_sfx);
            }

            inline void PopFolder() {
                this->cur_folder_path = fs::GetBaseDirectory(this->cur_folder_path);
                pu::audio::PlaySfx(this->close_folder_sfx);
            }

            inline void SetTopMenuDefault() {
                this->top_menu_default_bg->SetVisible(true);
                this->top_menu_folder_bg->SetVisible(false);
                this->top_menu_app_bg->SetVisible(false);
                this->top_menu_hb_bg->SetVisible(false);
            }

            inline void SetTopMenuFolder() {
                this->top_menu_default_bg->SetVisible(false);
                this->top_menu_folder_bg->SetVisible(true);
                this->top_menu_app_bg->SetVisible(false);
                this->top_menu_hb_bg->SetVisible(false);
            }

            inline void SetTopMenuApplication() {
                this->top_menu_default_bg->SetVisible(false);
                this->top_menu_folder_bg->SetVisible(false);
                this->top_menu_app_bg->SetVisible(true);
                this->top_menu_hb_bg->SetVisible(false);
            }

            inline void SetTopMenuHomebrew() {
                this->top_menu_default_bg->SetVisible(false);
                this->top_menu_folder_bg->SetVisible(false);
                this->top_menu_app_bg->SetVisible(false);
                this->top_menu_hb_bg->SetVisible(true);
            }

            void LaunchHomebrewApplication(const Entry &hb_entry);

        public:
            MainMenuLayout();
            PU_SMART_CTOR(MainMenuLayout)

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            void OnMenuUpdate() override;

            bool OnHomeButtonPress() override;
            void LoadSfx() override;
            void DisposeSfx() override;

            void MoveTo(const std::string &new_path, const bool fade, std::function<void()> action = nullptr);

            inline void MoveToRoot(const bool fade, std::function<void()> action = nullptr) {
                this->cur_folder_path = "";
                this->cur_path_text->SetText(this->cur_folder_path);
                this->MoveTo(GetActiveMenuPath(), fade, action);
            }

            inline void MoveEntryToParentFolder(const Entry &entry) {
                Entry entry_copy(entry);
                entry_copy.MoveToParentFolder();
                pu::audio::PlaySfx(this->entry_move_into_sfx);
                this->StopSelection();
                this->entry_menu->NotifyEntryRemoved(entry);
                this->entry_menu->OrganizeUpdateEntries();
            }

            inline void MoveEntryToRoot(const Entry &entry) {
                Entry entry_copy(entry);
                entry_copy.MoveToRoot(GetActiveMenuPath());
                pu::audio::PlaySfx(this->entry_move_into_sfx);
                this->StopSelection();
                this->entry_menu->NotifyEntryRemoved(entry);
                this->entry_menu->OrganizeUpdateEntries();
            }

            inline void RemoveEntry(const Entry &entry) {
                Entry entry_copy(entry);
                // New entries moved outside if the removed entry is a folder
                const auto new_entries = entry_copy.Remove();
                pu::audio::PlaySfx(this->entry_remove_sfx);
                for(const auto &new_entry: new_entries) {
                    this->entry_menu->NotifyEntryAdded(new_entry);
                }
                this->entry_menu->NotifyEntryRemoved(entry);
                this->entry_menu->OrganizeUpdateEntries();
            }

            inline void StartResume() {
                pu::audio::PlaySfx(this->resume_app_sfx);
                RequestResumeScreenCaptureBackground();
            }

            inline void UpdateApplicationVerifyProgress(const u64 app_id, const float progress) {
                const auto &cur_entries = this->entry_menu->GetEntries();
                for(u32 i = 0; i < cur_entries.size(); i++) {
                    const auto &entry = cur_entries.at(i);
                    if(entry.Is<EntryType::Application>() && (entry.app_info.app_id == app_id)) {
                        this->entry_menu->UpdateEntryProgress(i, progress);
                        break;
                    }
                }
            }

            void Initialize();

            void Reload();

            inline void NotifyNextReloadUserChanged() {
                this->next_reload_user_changed = true;
            }

            void HandleCloseSuspended();
            void HandleHomebrewLaunch(const Entry &entry);
            void StopSelection();
            void DoTerminateApplication();
    };

}
