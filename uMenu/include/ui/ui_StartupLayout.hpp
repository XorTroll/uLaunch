
#pragma once
#include <ul_Include.hpp>
#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <db/db_Save.hpp>
#include <ui/ui_IMenuLayout.hpp>

namespace ui {

    class StartupLayout : public IMenuLayout {

        private:
            bool loadmenu;
            pu::ui::elm::TextBlock::Ref infoText;
            pu::ui::elm::Menu::Ref usersMenu;

        public:
            StartupLayout();
            PU_SMART_CTOR(StartupLayout)
            
            void OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) override;
            bool OnHomeButtonPress() override;

            void user_Click(AccountUid uid);
            void create_Click();
            void ReloadMenu();

    };

}