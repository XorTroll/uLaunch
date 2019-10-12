
#pragma once
#include <q_Include.hpp>
#include <pu/Plutonium>
#include <ui/ui_SideMenu.hpp>
#include <ui/ui_RawData.hpp>
#include <cfg/cfg_Config.hpp>

namespace ui
{
    class MenuLayout : public pu::ui::Layout
    {
        public:
            MenuLayout(void *raw, u8 min_alpha, bool hb_mode);
            PU_SMART_CTOR(MenuLayout)

            void menu_Click(u64 down, u32 index);
            void MoveFolder(std::string name, bool fade);
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos);
            void ChangeMenuMode();

            bool HandleFolderChange(cfg::TitleRecord &rec);
        private:
            void *susptr;
            SideMenu::Ref itemsMenu;
            RawData::Ref bgSuspendedRaw;
            std::string curfolder;
            std::chrono::steady_clock::time_point tp;
            bool warnshown;
            bool homebrew_mode;
            u8 minalpha;
            u32 root_idx;
            u32 root_baseidx;
            u32 mode;
            s32 rawalpha;
    };
}