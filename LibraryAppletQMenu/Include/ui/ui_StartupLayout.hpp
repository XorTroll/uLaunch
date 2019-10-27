
#pragma once
#include <q_Include.hpp>
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
            void user_Click();
            void create_Click();
            void ReloadMenu();
        private:
            pu::ui::elm::TextBlock::Ref infoText;
            pu::ui::elm::Menu::Ref usersMenu;
            std::vector<u128> userlist;
            std::vector<bool> passlist;
    };
}