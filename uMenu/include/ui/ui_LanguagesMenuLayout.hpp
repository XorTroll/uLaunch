
#pragma once
#include <ul_Include.hpp>
#include <ui/ui_IMenuLayout.hpp>
#include <cfg/cfg_Config.hpp>

namespace ui {

    class LanguagesMenuLayout : public IMenuLayout {

        private:
            pu::ui::elm::TextBlock::Ref infoText;
            pu::ui::elm::Menu::Ref langsMenu;

        public:
            LanguagesMenuLayout();
            PU_SMART_CTOR(LanguagesMenuLayout)

            void OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) override;
            bool OnHomeButtonPress() override;

            void Reload();
            void lang_Click(u32 idx);
        
    };

}