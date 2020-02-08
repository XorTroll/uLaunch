
#pragma once
#include <ul_Include.hpp>
#include <am/am_DaemonMenuInteraction.hpp>
#include <db/db_Save.hpp>
#include <ui/ui_IMenuLayout.hpp>

namespace ui
{
    class StartupLayout : public IMenuLayout
    {
        public:
            StartupLayout();
            PU_SMART_CTOR(StartupLayout)
            
            void OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch pos) override;
            void OnHomeButtonPress() override;

            void user_Click(AccountUid uid);
            void create_Click();
            void ReloadMenu();
        private:
            bool loadmenu;
            pu::ui::elm::TextBlock::Ref infoText;
            pu::ui::elm::Menu::Ref usersMenu;
    };
}