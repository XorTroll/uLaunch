
#pragma once
#include <ul_Include.hpp>
#include <pu/Plutonium>
#include <cfg/cfg_Config.hpp>

namespace ui
{
    class LanguagesMenuLayout : public pu::ui::Layout
    {
        public:
            LanguagesMenuLayout();
            PU_SMART_CTOR(LanguagesMenuLayout)
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos);
            void Reload();
            void lang_Click(u32 idx);
        private:
            pu::ui::elm::TextBlock::Ref infoText;
            pu::ui::elm::Menu::Ref langsMenu;
    };
}