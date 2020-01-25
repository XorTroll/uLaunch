
#pragma once
#include <ul_Include.hpp>
#include <pu/Plutonium>
#include <cfg/cfg_Config.hpp>

namespace ui
{
    class SettingsMenuLayout : public pu::ui::Layout
    {
        public:
            SettingsMenuLayout();
            PU_SMART_CTOR(SettingsMenuLayout)
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos);
            void Reload();
            void PushSettingItem(std::string name, std::string value_display, int id);
            void setting_Click(u32 id);
        private:
            pu::ui::elm::TextBlock::Ref infoText;
            pu::ui::elm::Menu::Ref settingsMenu;
    };
}