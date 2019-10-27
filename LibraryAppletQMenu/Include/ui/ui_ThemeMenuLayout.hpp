
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
            std::vector<cfg::Theme> loadedThemes;
    };
}