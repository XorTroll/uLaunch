
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/menu/ui/ui_RawRgbaImage.hpp>
#include <ul/menu/ui/ui_ClickableImage.hpp>
#include <ul/menu/ui/ui_QuickMenu.hpp>
#include <ul/menu/ui/ui_InputBar.hpp>
#include <ul/menu/ui/ui_EntryMenu.hpp>
#include <ul/menu/ui/ui_Actions.hpp>
#include <ul/menu/menu_Entries.hpp>
#include <ul/cfg/cfg_Config.hpp>

namespace ul::menu::ui {

    class MainMenuLayout : public IMenuLayout {
        public:
            static constexpr u8 SuspendedScreenAlphaIncrement = 6;

        private:
            enum class SuspendedImageMode {
                ShowingAfterStart = 0,
                Focused = 1,
                HidingForResume = 2,
                NotFocused = 3,
                ShowingGainedFocus = 4,
                HidingLostFocus = 5
            };

            bool last_has_connection;
            u32 last_battery_lvl;
            bool last_is_charging;
            pu::ui::elm::Image::Ref top_menu_img;
            pu::ui::elm::Image::Ref connection_icon;
            ClickableImage::Ref users_img;
            ClickableImage::Ref controller_img;
            ClickableImage::Ref logo_img;
            pu::ui::elm::TextBlock::Ref time_text;
            pu::ui::elm::TextBlock::Ref battery_text;
            pu::ui::elm::Image::Ref battery_icon;
            ClickableImage::Ref settings_img;
            ClickableImage::Ref themes_img;
            pu::ui::elm::TextBlock::Ref fw_text;
            EntryMenu::Ref entries_menu;
            std::string cur_folder_path;
            RawRgbaImage::Ref suspended_screen_img;
            pu::ui::elm::TextBlock::Ref selected_item_name_text;
            pu::ui::elm::TextBlock::Ref selected_item_author_text;
            pu::ui::elm::TextBlock::Ref selected_item_version_text;
            pu::ui::elm::Image::Ref banner_img;
            pu::ui::elm::TextBlock::Ref no_entries_text;
            pu::ui::elm::TextBlock::Ref cur_path_text;
            QuickMenu::Ref quick_menu;
            InputBar::Ref input_bar;
            std::chrono::steady_clock::time_point startup_tp;
            bool launch_fail_warn_shown;
            u8 min_alpha;
            u8 target_alpha;
            SuspendedImageMode mode;
            s32 suspended_screen_alpha;
            pu::audio::Sfx title_launch_sfx;

            void DoMoveTo(const std::string &new_path);
            void menu_EntryInputPressed(const u64 keys_down);
            void menu_FocusedEntryChanged(const bool has_prev_entry, const bool is_prev_entry_suspended, const bool is_cur_entry_suspended);

            inline void PushFolder(const std::string &name) {
                this->cur_folder_path = fs::JoinPath(this->cur_folder_path, name);
                this->cur_path_text->SetText(this->cur_folder_path);
            }

            inline void PopFolder() {
                this->cur_folder_path = fs::GetBaseDirectory(this->cur_folder_path);
                this->cur_path_text->SetText(this->cur_folder_path);
            }

        public:
            MainMenuLayout(const u8 *captured_screen_buf, const u8 min_alpha);
            ~MainMenuLayout();
            PU_SMART_CTOR(MainMenuLayout)

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;

            void MoveTo(const std::string &new_path, const bool fade, std::function<void()> action = nullptr);
            void SetUser(const AccountUid user);
            void HandleCloseSuspended();
            void HandleHomebrewLaunch(const Entry &entry);
            void HandleMultiselectMoveToFolder(const std::string &new_path);
            void StopSelection();
            void DoTerminateApplication();
    };

}