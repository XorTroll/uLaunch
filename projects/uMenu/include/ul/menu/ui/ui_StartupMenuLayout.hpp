
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>

namespace ul::menu::ui {

    class StartupMenuLayout : public IMenuLayout {
        public:
            static constexpr u32 UsersMenuWidth = 1320;
            static constexpr u32 UsersMenuItemSize = 150;
            static constexpr u32 UsersMenuItemsToShow = 5;

        private:
            bool load_menu;
            pu::ui::elm::TextBlock::Ref info_text;
            pu::ui::elm::Menu::Ref users_menu;
            pu::audio::Sfx user_create_sfx;
            pu::audio::Sfx user_select_sfx;

            void user_DefaultKey(const AccountUid uid);
            void create_DefaultKey();

        public:
            StartupMenuLayout();
            PU_SMART_CTOR(StartupMenuLayout)
            ~StartupMenuLayout();
            
            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;

            void ReloadMenu();
    };

}
