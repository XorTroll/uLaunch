
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/menu/ui/ui_SideMenu.hpp>
#include <ul/menu/ui/ui_RawRgbaImage.hpp>
#include <ul/menu/ui/ui_ClickableImage.hpp>
#include <ul/menu/ui/ui_QuickMenu.hpp>
#include <ul/menu/ui/ui_Actions.hpp>
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
            SideMenu::Ref items_menu;
            RawRgbaImage::Ref suspended_screen_img;
            pu::ui::elm::TextBlock::Ref selected_item_name_text;
            pu::ui::elm::TextBlock::Ref selected_item_author_text;
            pu::ui::elm::TextBlock::Ref selected_item_version_text;
            pu::ui::elm::Image::Ref banner_img;
            pu::ui::elm::Image::Ref guide_buttons_img;
            ClickableImage::Ref menu_totggle_img;
            QuickMenu::Ref quick_menu;
            std::string cur_folder;
            std::chrono::steady_clock::time_point startup_tp;
            bool launch_fail_warn_shown;
            bool homebrew_mode;
            bool select_on;
            bool select_dir;
            u8 min_alpha;
            u8 target_alpha;
            SuspendedImageMode mode;
            s32 suspended_screen_alpha;
            pu::audio::Sfx title_launch_sfx;
            pu::audio::Sfx menu_toggle_sfx;

            void DoMoveFolder(const std::string &name);

            void menu_Click(const u64 keys_down, const u32 idx);
            void menu_OnSelected(const s32 prev_idx, const u32 idx);
            void menuToggle_Click();

        public:
            MainMenuLayout(const u8 *captured_screen_buf, const u8 min_alpha);
            ~MainMenuLayout();
            PU_SMART_CTOR(MainMenuLayout)

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;

            void MoveFolder(const std::string &name, const bool fade);
            void SetUser(const AccountUid user);
            void HandleCloseSuspended();
            void HandleHomebrewLaunch(const cfg::TitleRecord &rec);
            void HandleMultiselectMoveToFolder(const std::string &folder);
            void StopMultiselect();
            void DoTerminateApplication();
    };

}