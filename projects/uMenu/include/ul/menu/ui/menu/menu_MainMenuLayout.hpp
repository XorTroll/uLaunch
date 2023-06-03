
#pragma once
#include <ul/menu/ui/menu/menu_MenuLayout.hpp>
#include <ul/menu/ui/ui_EntryMenu.hpp>
#include <ul/menu/ui/ui_QuickMenu.hpp>
#include <ul/menu/ui/ui_InputBar.hpp>
#include <ul/menu/ui/ui_ClickableImage.hpp>
#include <ul/menu/ui/ui_RawRgbaImage.hpp>
#include <ul/util/util_Enum.hpp>

namespace ul::menu::ui::menu {

    enum class InputStatus : u32 {
        Normal = 0,
        InFolder = BIT(0),
        CurrentEntryInvalid = BIT(1),
        CurrentEntryFolder = BIT(2),
        CurrentEntrySuspended = BIT(3),
        EntrySelected = BIT(4)
    };
    UL_UTIL_ENUM_DEFINE_FLAG_OPERATORS(InputStatus, u32)

    class MainMenuLayout : public MenuLayout {
        public:
            static constexpr u8 SuspendedScreenAlphaFullGoal = 0xFF;
            static constexpr u8 SuspendedScreenAlphaNoneGoal = 0;
            static constexpr u8 SuspendedScreenAlphaSuspendedGoal = 120;
            static constexpr u8 SuspendedScreenAlphaIncrement = 10;

        private:
            bool cur_has_connection;
            u32 cur_battery_lvl;
            bool cur_is_charging;
            pu::ui::elm::Image::Ref top_menu_img;
            ClickableImage::Ref logo_img;
            pu::ui::elm::Image::Ref connection_icon;
            ClickableImage::Ref users_img;
            ClickableImage::Ref controller_img;
            ClickableImage::Ref mii_img;
            ClickableImage::Ref amiibo_img;
            pu::ui::elm::TextBlock::Ref time_text;
            pu::ui::elm::TextBlock::Ref battery_text;
            pu::ui::elm::Image::Ref battery_icon;
            ClickableImage::Ref settings_img;
            ClickableImage::Ref themes_img;
            pu::ui::elm::TextBlock::Ref fw_text;
            RawRgbaImage::Ref suspended_screen_img;
            const u8 *captured_screen_buf;
            std::chrono::steady_clock::time_point startup_tp;
            bool launch_failure_warn_shown;
            u8 min_alpha;
            u32 mode;
            std::function<void()> goal_reach_cb;
            u8 suspended_screen_alpha_goal;
            s32 suspended_screen_alpha;
            pu::audio::Sfx title_launch_sfx;
            pu::audio::Sfx menu_toggle_sfx;

            EntryMenu::Ref entry_menu;
            InputBar::Ref input_bar;
            QuickMenu::Ref quick_menu;
            InputStatus status;

            inline void VariateSuspendedScreenAlphaTo(const u8 alpha_goal) {
                if(this->suspended_screen_alpha > alpha_goal) {
                    this->suspended_screen_alpha -= SuspendedScreenAlphaIncrement;
                    if(this->suspended_screen_alpha < alpha_goal) {
                        this->suspended_screen_alpha = alpha_goal;
                    }
                }
                else if(this->suspended_screen_alpha < alpha_goal) {
                    this->suspended_screen_alpha += SuspendedScreenAlphaIncrement;
                    if(this->suspended_screen_alpha > alpha_goal) {
                        this->suspended_screen_alpha = alpha_goal;
                    }
                }
            }

        public:
            MainMenuLayout(const u8 *captured_screen_buf, const u8 min_alpha);
            ~MainMenuLayout();
            PU_SMART_CTOR(MainMenuLayout)

            void UpdateInput(const InputStatus status);

            inline void UpdateInputIfDifferent(const InputStatus status) {
                if(status != this->status) {
                    this->UpdateInput(status);
                }
            }

            inline void ToggleQuickMenu() {
                this->quick_menu->Toggle();
            }

            void NotifyResume();

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;
    };

}