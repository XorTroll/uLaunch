
#pragma once
#include <q_Include.hpp>
#include <pu/Plutonium>
#include <cfg/cfg_Config.hpp>

namespace ui
{
    class ThemeMenuLayout : public pu::ui::Layout
    {
        public:
            ThemeMenuLayout();
            PU_SMART_CTOR(ThemeMenuLayout)
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos);
            void Reload();
            void theme_Click();
        private:
            pu::ui::elm::TextBlock::Ref noThemesText;
            pu::ui::elm::Menu::Ref themesMenu;
            pu::ui::elm::TextBlock::Ref curThemeText;
            pu::ui::elm::TextBlock::Ref curThemeName;
            pu::ui::elm::TextBlock::Ref curThemeAuthor;
            pu::ui::elm::TextBlock::Ref curThemeVersion;
            pu::ui::elm::Image::Ref curThemeIcon;
            pu::ui::elm::Image::Ref curThemeBanner;
            std::vector<cfg::Theme> loadedThemes;
    };
}