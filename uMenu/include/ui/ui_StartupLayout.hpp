
#pragma once
#include <ul_Include.hpp>
#include <am/am_QCommunications.hpp>
#include <db/db_Save.hpp>
#include <pu/Plutonium>

namespace ui
{
    class StartupLayout : public pu::ui::Layout
    {
        public:
            StartupLayout();
            PU_SMART_CTOR(StartupLayout)
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos);
            void user_Click(AccountUid uid, bool has_password);
            void create_Click();
            void ReloadMenu();
        private:
            bool loadmenu;
            pu::ui::elm::TextBlock::Ref infoText;
            pu::ui::elm::Menu::Ref usersMenu;
    };
}