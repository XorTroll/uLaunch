
#pragma once
#include <pu/Plutonium>
#include <map>

namespace ui
{
    class SideMenu : public pu::ui::elm::Element
    {
        static constexpr u32 ItemSize = 256;
        static constexpr u32 Margin = 20;
        static constexpr u32 ItemCount = 4;
        static constexpr u32 FocusSize = 15;
        static constexpr u32 FocusMargin = 5;
        static constexpr u32 ExtraIconSize = ItemSize + (Margin * 2);

        public:
            SideMenu(pu::ui::Color suspended_clr, std::string cursor_path, std::string suspended_img_path, std::string multiselect_img_path, u32 txt_x, u32 txt_y, pu::String font_name, pu::ui::Color txt_clr, s32 y);
            PU_SMART_CTOR(SideMenu)
            ~SideMenu();

            s32 GetX() override;
            s32 GetY() override;
            void SetX(s32 x); // Stubbed
            void SetY(s32 y);
            s32 GetWidth() override;
            s32 GetHeight() override;
            void OnRender(pu::ui::render::Renderer::Ref &drawer, s32 x, s32 y) override;
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos) override;
            void SetOnItemSelected(std::function<void(u64, u32)> fn);
            void SetOnSelectionChanged(std::function<void(u32)> fn);
            void ClearItems();
            void AddItem(const std::string &icon, const std::string &txt = "");
            void SetSuspendedItem(u32 idx);
            void UnsetSuspendedItem();
            void SetSelectedItem(u32 idx);
            void HandleMoveLeft();
            void HandleMoveRight();
            int GetSuspendedItem();
            u32 GetSelectedItem();
            u32 GetBaseItemIndex();
            void SetBaseItemIndex(u32 idx);
            void SetBasePositions(u32 selected_idx, u32 base_idx);
            void ClearBorderIcons();
            void UpdateBorderIcons();
            void ResetMultiselections();
            void SetItemMultiselected(u32 idx, bool selected);
            bool IsItemMultiselected(u32 idx);
            bool IsAnyMultiselected();
            void SetEnabled(bool enabled);
        private:
            s32 y;
            u32 selitm;
            u32 preselitm;
            int suspitm;
            u32 baseiconidx;
            u8 movalpha;
            u32 textx;
            u32 texty;
            bool enabled;
            pu::ui::Color suspclr;
            pu::ui::Color textclr;
            std::vector<std::string> icons;
            std::vector<std::string> icons_texts;
            std::vector<bool> icons_mselected;
            std::function<void(u64, u32)> onselect;
            std::function<void(u32)> onselch;
            std::vector<pu::sdl2::Texture> ricons;
            std::vector<pu::sdl2::Texture> ricons_texts;
            pu::sdl2::Texture cursoricon;
            pu::sdl2::Texture suspicon;
            pu::sdl2::Texture leftbicon;
            pu::sdl2::Texture rightbicon;
            pu::sdl2::Texture mselicon;
            pu::String textfont;
            std::chrono::steady_clock::time_point scrolltp;
            bool scrollmoveflag;
            std::chrono::steady_clock::time_point scrollmovetp;
            u32 scrollflag;
            u32 scrolltpvalue;
            u32 scrollcount;
            bool IsLeftFirst();
            bool IsRightLast();
            void ReloadIcons(u32 dir);
    };
}