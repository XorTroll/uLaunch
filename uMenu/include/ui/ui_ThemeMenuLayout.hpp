
#pragma once
#include <ul_Include.hpp>
#include <ui/ui_IMenuLayout.hpp>
#include <cfg/cfg_Config.hpp>

namespace ui {

    class ThemeMenuLayout : public IMenuLayout {

        private:
            pu::ui::elm::Menu::Ref themesMenu;
            pu::ui::elm::TextBlock::Ref curThemeText;
            pu::ui::elm::TextBlock::Ref curThemeName;
            pu::ui::elm::TextBlock::Ref curThemeAuthor;
            pu::ui::elm::TextBlock::Ref curThemeVersion;
            pu::ui::elm::Image::Ref curThemeIcon;
            pu::ui::elm::Image::Ref curThemeBanner;
            std::vector<cfg::Theme> loadedThemes;

        public:
            ThemeMenuLayout();
            PU_SMART_CTOR(ThemeMenuLayout)

            void OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) override;
            void OnHomeButtonPress() override;

            void Reload();
            void theme_Click();

    };

}