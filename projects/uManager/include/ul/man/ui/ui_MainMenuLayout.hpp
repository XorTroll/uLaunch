
#pragma once
#include <pu/Plutonium>

namespace ul::man::ui {

    class MainMenuLayout : public pu::ui::Layout {
        public:
            static constexpr u32 MenuItemCount = 6;

        private:
            pu::ui::elm::TextBlock::Ref info_text;

            pu::ui::elm::Menu::Ref options_menu;
            pu::ui::elm::MenuItem::Ref activate_menu_item;
            pu::ui::elm::MenuItem::Ref update_menu_item;

            inline void ResetInfoText() {
                this->info_text->SetText("uManager v" UL_VERSION " - uLaunch's manager");
            }

        public:
            MainMenuLayout();
            PU_SMART_CTOR(MainMenuLayout)

            void activate_DefaultKey();
            void update_DefaultKey();
    };

}