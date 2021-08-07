
#pragma once
#include <ul_Include.hpp>
#include <ui/ui_IMenuLayout.hpp>
#include <ui/ui_SideMenu.hpp>
#include <ui/ui_RawData.hpp>
#include <ui/ui_ClickableImage.hpp>
#include <ui/ui_QuickMenu.hpp>
#include <ui/ui_Actions.hpp>
#include <cfg/cfg_Config.hpp>

namespace ui {

    class MenuLayout : public IMenuLayout {

        private:
            void *susptr;
            bool last_hasconn;
            u32 last_batterylvl;
            bool last_charge;
            pu::ui::elm::Image::Ref topMenuImage;
            pu::ui::elm::Image::Ref connIcon;
            ClickableImage::Ref users;
            ClickableImage::Ref controller;
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
            pu::ui::elm::Image::Ref guideButtons;
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

            inline void ApplySuspendedRatio(bool increase) {
                auto susp_w = this->bgSuspendedRaw->GetWidth();
                auto susp_h = this->bgSuspendedRaw->GetHeight();
                // Change size, 16:9 ratio
                if(increase) {
                    susp_w += 16;
                    susp_h += 9;
                }
                else {
                    susp_w -= 16;
                    susp_h -= 9;
                }
                auto susp_x = (1280 - susp_w) / 2;
                auto susp_y = (720 - susp_h) / 2;
                this->bgSuspendedRaw->SetX(susp_x);
                this->bgSuspendedRaw->SetY(susp_y);
                this->bgSuspendedRaw->SetWidth(susp_w);
                this->bgSuspendedRaw->SetHeight(susp_h);
            }

        public:
            MenuLayout(void *raw, u8 min_alpha);
            ~MenuLayout();
            PU_SMART_CTOR(MenuLayout)

            void OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) override;
            bool OnHomeButtonPress() override;

            void menu_Click(u64 down, u32 index);
            void menu_OnSelected(u32 index);
            void menuToggle_Click();
            void MoveFolder(const std::string &name, bool fade);
            void SetUser(AccountUid user);
            void HandleCloseSuspended();
            void HandleHomebrewLaunch(cfg::TitleRecord &rec);
            void HandleMultiselectMoveToFolder(const std::string &folder);
            void StopMultiselect();
            void DoTerminateApplication();

    };

}