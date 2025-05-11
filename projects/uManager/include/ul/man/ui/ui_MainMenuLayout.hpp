
#pragma once
#include <pu/Plutonium>

namespace ul::man::ui {

    class MainMenuLayout : public pu::ui::Layout {
        public:
            static constexpr pu::ui::Color BackgroundColor = { 61, 164, 255, 255 };
            
            static constexpr u32 InfoTextY = 50;
            static constexpr pu::ui::Color InfoTextColor = { 255, 255, 255, 255 };

            static constexpr u32 UpdateDownloadBarY = 170;
            static constexpr u32 UpdateDownloadBarHorizontalMargin = 240;
            static constexpr u32 UpdateDownloadBarWidth = pu::ui::render::ScreenWidth - 2 * UpdateDownloadBarHorizontalMargin;
            static constexpr u32 UpdateDownloadBarHeight = 80;
            
            static constexpr u32 MenuY = 160;
            static constexpr pu::ui::Color MenuColor = { 4, 109, 208, 255 };
            static constexpr pu::ui::Color MenuFocusColor = { 0, 148, 255, 255 };
            static constexpr u32 MenuItemCount = 6;
            static constexpr u32 MenuItemSize = 120;
            static constexpr pu::ui::Color MenuItemColor = { 225, 225, 225, 255 };

        private:
            pu::ui::elm::TextBlock::Ref info_text;
            pu::ui::elm::ProgressBar::Ref update_download_bar;
            pu::ui::elm::Menu::Ref options_menu;
            pu::ui::elm::MenuItem::Ref activate_menu_item;
            pu::ui::elm::MenuItem::Ref reset_menu_menu_item;
            pu::ui::elm::MenuItem::Ref reset_cache_menu_item;
            pu::ui::elm::MenuItem::Ref update_menu_item;

            void ResetInfoText();

            inline void ReloadMenu() {
                this->options_menu->ClearItems();
                this->options_menu->AddItem(this->activate_menu_item);
                this->options_menu->AddItem(this->reset_menu_menu_item);
                this->options_menu->AddItem(this->reset_cache_menu_item);
                this->options_menu->AddItem(this->update_menu_item);
            }

        public:
            MainMenuLayout();
            PU_SMART_CTOR(MainMenuLayout)

            void activate_DefaultKey();
            void resetMenu_DefaultKey();
            void resetCache_DefaultKey();
            void update_DefaultKey();
    };

}
