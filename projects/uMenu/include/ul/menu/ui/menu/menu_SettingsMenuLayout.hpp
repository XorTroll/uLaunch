
#pragma once
#include <ul/menu/ui/menu/menu_MenuLayout.hpp>
#include <ul/menu/ui/ui_InputBar.hpp>

namespace ul::menu::ui::menu {

    struct SettingsEntry {
        std::string name;
        std::function<std::string()> get_value_fn;
        std::function<bool()> on_select_fn;

        SettingsEntry(const std::string &name, std::function<std::string()> get_value_fn, std::function<bool()> on_select_fn) : name(name), get_value_fn(get_value_fn), on_select_fn(on_select_fn) {}

        inline std::string GetDescription() const {
            const auto value = this->get_value_fn();
            if(value.empty()) {
                return this->name;
            }
            else {
                return this->name + ": " + value;
            }
        }

        inline bool Select() const {
            return this->on_select_fn();
        }
    };

    class SettingsMenuLayout : public MenuLayout {
        private:
            pu::ui::elm::TextBlock::Ref info_text;
            pu::ui::elm::Menu::Ref settings_menu;
            InputBar::Ref input_bar;
            std::vector<SettingsEntry> entries;

            void entry_DefaultKey(const u32 entry_idx);
            void ReloadEntries(const bool rewind_menu);

        public:
            SettingsMenuLayout();
            PU_SMART_CTOR(SettingsMenuLayout)

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;
    };

}