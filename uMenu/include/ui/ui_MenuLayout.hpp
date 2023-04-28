
#pragma once
#include <ul_Include.hpp>
#include <ui/ui_IMenuLayout.hpp>
#include <ui/ui_SideMenu.hpp>
#include <ui/ui_RawRgbaImage.hpp>
#include <ui/ui_ClickableImage.hpp>
#include <ui/ui_QuickMenu.hpp>
#include <ui/ui_Actions.hpp>
#include <cfg/cfg_Config.hpp>

namespace ui {

    class MenuLayout : public IMenuLayout {
        public:
            // 16:9 ratio
            static constexpr u32 SuspendedImageWidthIncrement = (u32)30.0f;
            static constexpr u32 SuspendedImageHeightIncrement = (u32)(SuspendedImageWidthIncrement / (16.0f / 9.0f));

            static constexpr u8 SuspendedScreenAlphaIncrement = 10;

        private:
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
            u32 mode;
            s32 suspended_screen_alpha;
            pu::audio::Sfx title_launch_sfx; //When launching a title
            pu::audio::Sfx menu_toggle_sfx; //When toggling HB menu and default menu and viceversa
            pu::audio::Sfx title_select_sfx; //When scrolling titles

            void DoMoveFolder(const std::string &name);

            void menu_Click(const u64 keys_down, const u32 idx);
            void menu_OnSelected(const u32 idx);
            void menuToggle_Click();

            inline void ApplySuspendedRatio(const bool increase) {
                auto susp_w = this->suspended_screen_img->GetWidth();
                auto susp_h = this->suspended_screen_img->GetHeight();

                if(increase) {
                    susp_w += (s32)SuspendedImageWidthIncrement;
                    susp_h += (s32)SuspendedImageHeightIncrement;
                }
                else {
                    susp_w -= (s32)SuspendedImageWidthIncrement;
                    susp_h -= (s32)SuspendedImageHeightIncrement;
                }
                
                const auto susp_x = (pu::ui::render::ScreenWidth - susp_w) / 2;
                const auto susp_y = (pu::ui::render::ScreenHeight - susp_h) / 2;
                this->suspended_screen_img->SetX(susp_x);
                this->suspended_screen_img->SetY(susp_y);
                this->suspended_screen_img->SetWidth(susp_w);
                this->suspended_screen_img->SetHeight(susp_h);
            }

        public:
            MenuLayout(const u8 *captured_screen_buf, const u8 min_alpha);
            ~MenuLayout();
            PU_SMART_CTOR(MenuLayout)

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