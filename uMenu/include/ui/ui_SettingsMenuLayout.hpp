
#pragma once
#include <ul_Include.hpp>
#include <ui/ui_IMenuLayout.hpp>
#include <cfg/cfg_Config.hpp>

namespace ui {

    class SettingsMenuLayout : public IMenuLayout {

        private:
            pu::ui::elm::TextBlock::Ref infoText;
            pu::ui::elm::Menu::Ref settingsMenu;

        public:
            SettingsMenuLayout();
            PU_SMART_CTOR(SettingsMenuLayout)

            void OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) override;
            void OnHomeButtonPress() override;

            void Reload();
            void PushSettingItem(const std::string &name, const std::string &value_display, int id);
            void setting_Click(u32 id);

    };

}