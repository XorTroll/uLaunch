
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>

namespace ul::menu::ui {

    class StartupLayout : public IMenuLayout {
        private:
            bool load_menu;
            pu::ui::elm::TextBlock::Ref info_text;
            pu::ui::elm::Menu::Ref users_menu;

            void user_DefaultKey(const AccountUid uid);
            void create_DefaultKey();

        public:
            StartupLayout();
            PU_SMART_CTOR(StartupLayout)
            
            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;

            void ReloadMenu();
    };

}