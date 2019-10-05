
#pragma once
#include <q_Include.hpp>
#include <am/am_QCommunications.hpp>
#include <db/db_Save.hpp>
#include <pu/Plutonium>
#include <ui/ui_RawData.hpp>

namespace ui
{
    class StartupLayout : public pu::ui::Layout
    {
        public:
            StartupLayout(pu::ui::Color bgcolor);
            PU_SMART_CTOR(StartupLayout)

            void user_Click();
        private:
            pu::ui::elm::TextBlock::Ref infoText;
            pu::ui::elm::Menu::Ref usersMenu;
            std::vector<u128> userlist;
            std::vector<bool> passlist;
    };
}