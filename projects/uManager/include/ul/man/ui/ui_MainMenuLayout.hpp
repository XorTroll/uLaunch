
#pragma once
#include <pu/Plutonium>

namespace ul::man::ui {

    class MainMenuLayout : public pu::ui::Layout {
        public:
            static constexpr u32 MenuItemCount = 6;

        private:
            pu::ui::elm::Menu::Ref options_menu;
            pu::ui::elm::TextBlock::Ref info_text;
            pu::ui::elm::MenuItem::Ref activate_menu_item;
        public:
            MainMenuLayout();
            PU_SMART_CTOR(MainMenuLayout)

            void activate_DefaultKey();
    };

}