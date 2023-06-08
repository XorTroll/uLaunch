
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/cfg/cfg_Config.hpp>

namespace ul::menu::ui {

    class ThemeMenuLayout : public IMenuLayout {
        private:
            pu::ui::elm::Menu::Ref themes_menu;
            pu::ui::elm::TextBlock::Ref cur_theme_text;
            pu::ui::elm::TextBlock::Ref cur_theme_name_text;
            pu::ui::elm::TextBlock::Ref cur_theme_author_text;
            pu::ui::elm::TextBlock::Ref cur_theme_version_text;
            pu::ui::elm::Image::Ref cur_theme_icon;
            pu::ui::elm::Image::Ref cur_theme_banner;
            std::vector<cfg::Theme> loaded_themes;

            void theme_DefaultKey();

        public:
            ThemeMenuLayout();
            PU_SMART_CTOR(ThemeMenuLayout)

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;

            void Reload();
    };

}