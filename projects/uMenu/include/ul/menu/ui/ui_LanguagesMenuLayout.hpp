
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>

namespace ul::menu::ui {

    class LanguagesMenuLayout : public IMenuLayout {
        private:
            pu::ui::elm::TextBlock::Ref info_text;
            pu::ui::elm::Menu::Ref langs_menu;

            void lang_DefaultKey(const u32 idx);

        public:
            LanguagesMenuLayout();
            PU_SMART_CTOR(LanguagesMenuLayout)

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;

            void Reload();
    };

}