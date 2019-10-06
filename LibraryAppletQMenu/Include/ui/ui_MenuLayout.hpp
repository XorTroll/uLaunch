
#pragma once
#include <q_Include.hpp>
#include <pu/Plutonium>
#include <ui/ui_SideMenu.hpp>
#include <ui/ui_RawData.hpp>

namespace ui
{
    class MenuLayout : public pu::ui::Layout
    {
        public:
            MenuLayout(void *raw);
            PU_SMART_CTOR(MenuLayout)

            void menu_Click(u32 index);
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos);
            void SetSuspendedRawData(void *raw);
        private:
            void *susptr;
            SideMenu::Ref itemsMenu;
            RawData::Ref bgSuspendedRaw;
            u32 mode;
            s32 rawalpha;
    };
}