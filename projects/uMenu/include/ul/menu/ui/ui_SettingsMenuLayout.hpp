
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/cfg/cfg_Config.hpp>

namespace ul::menu::ui {

    class SettingsMenuLayout : public IMenuLayout {
        private:
            pu::ui::elm::TextBlock::Ref info_text;
            pu::ui::elm::Menu::Ref settings_menu;

            void PushSettingItem(const std::string &name, const std::string &value_display, const int id);
            void setting_DefaultKey(const u32 id);

        public:
            SettingsMenuLayout();
            PU_SMART_CTOR(SettingsMenuLayout)

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;

            void Reload(const bool reset_idx);
    };

}