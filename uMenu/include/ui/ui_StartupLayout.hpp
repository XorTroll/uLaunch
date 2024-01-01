
#pragma once
#include <ui/ui_IMenuLayout.hpp>

namespace ui {

    class StartupLayout : public IMenuLayout {
        private:
            bool load_menu;
            pu::ui::elm::TextBlock::Ref info_text;
            pu::ui::elm::Menu::Ref users_menu;
            pu::audio::Sfx user_scroll_sfx; //When scrolling users
            pu::audio::Sfx user_select_sfx; //When selecting an user
            JSON bgm_json;
            bool bgm_loop;
            u32 bgm_fade_in_ms;
            u32 bgm_fade_out_ms;
            pu::audio::Music bgm; //Bgm played when in the login screen

            void user_DefaultKey(const AccountUid uid);
            void create_DefaultKey();

        public:
            StartupLayout();
            PU_SMART_CTOR(StartupLayout)
            
            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;
            void StartPlayBGM();
            void StopPlayBGM();
            void ReloadMenu();
    };

}