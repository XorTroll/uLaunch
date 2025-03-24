
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/cfg/cfg_Config.hpp>

namespace ul::menu::ui {

    class ThemesMenuLayout : public IMenuLayout {
        public:
            static constexpr u32 ThemesMenuWidth = 1720;
            static constexpr u32 ThemesMenuItemSize = 180;
            static constexpr u32 ThemesMenuItemsToShow = 5;

        private:
            pu::ui::elm::Menu::Ref themes_menu;
            pu::ui::elm::TextBlock::Ref info_text;
            std::vector<cfg::Theme> loaded_themes;
            std::vector<pu::sdl2::TextureHandle::Ref> loaded_theme_icons;
            pu::audio::Sfx theme_change_sfx;
            pu::audio::Sfx back_sfx;

            void theme_DefaultKey();

        public:
            ThemesMenuLayout();
            PU_SMART_CTOR(ThemesMenuLayout)

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;
            void LoadSfx() override;
            void DisposeSfx() override;

            void Reload();
    };

}
