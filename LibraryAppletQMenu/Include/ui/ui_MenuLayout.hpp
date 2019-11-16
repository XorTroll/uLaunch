
#pragma once
#include <q_Include.hpp>
#include <pu/Plutonium>
#include <ui/ui_SideMenu.hpp>
#include <ui/ui_RawData.hpp>
#include <ui/ui_ClickableImage.hpp>
#include <ui/ui_QuickMenu.hpp>
#include <cfg/cfg_Config.hpp>

namespace ui
{
    class MenuLayout : public pu::ui::Layout
    {
        public:
            MenuLayout(void *raw, u8 min_alpha);
            ~MenuLayout();
            PU_SMART_CTOR(MenuLayout)

            void menu_Click(u64 down, u32 index);
            void menu_OnSelected(u32 index);
            void menuToggle_Click();
            void logo_Click();
            void settings_Click();
            void themes_Click();
            void users_Click();
            void web_Click();
            void MoveFolder(std::string name, bool fade);
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos);
            void SetUser(u128 user);
            void HandleCloseSuspended();
            void HandleHomebrewLaunch(cfg::TitleRecord &rec);
            void HandleUserMenu();
            void HandleWebPageOpen();
            void HandleSettingsMenu();
            void HandleThemesMenu();
            void HandleControllerAppletOpen();
            void HandleShowHelp();
            void HandleMultiselectMoveToFolder(std::string folder);
            void StopMultiselect();
        private:
            void *susptr;
            bool last_hasconn;
            u32 last_batterylvl;
            bool last_charge;
            pu::ui::elm::Image::Ref topMenuImage;
            pu::ui::elm::Image::Ref connIcon;
            ClickableImage::Ref users;
            ClickableImage::Ref web;
            ClickableImage::Ref logo;
            pu::ui::elm::TextBlock::Ref timeText;
            pu::ui::elm::TextBlock::Ref batteryText;
            pu::ui::elm::Image::Ref batteryIcon;
            ClickableImage::Ref settings;
            ClickableImage::Ref themes;
            pu::ui::elm::TextBlock::Ref fwText;
            SideMenu::Ref itemsMenu;
            RawData::Ref bgSuspendedRaw;
            pu::ui::elm::TextBlock::Ref itemName;
            pu::ui::elm::TextBlock::Ref itemAuthor;
            pu::ui::elm::TextBlock::Ref itemVersion;
            pu::ui::elm::Image::Ref bannerImage;
            ClickableImage::Ref menuToggle;
            QuickMenu::Ref quickMenu;
            std::string curfolder;
            std::chrono::steady_clock::time_point tp;
            bool warnshown;
            bool homebrew_mode;
            bool select_on;
            bool select_dir;
            u8 minalpha;
            u32 mode;
            s32 rawalpha;
            pu::audio::Sfx sfxTitleLaunch;
            pu::audio::Sfx sfxMenuToggle;
    };
}