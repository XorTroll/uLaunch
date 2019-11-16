
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
            SideMenu(pu::ui::Color SuspendedColor, std::string CursorPath, std::string SuspendedImagePath, std::string MultiselectImagePath, u32 TextX, u32 TextY, u32 TextSize, pu::ui::Color TextColor, s32 y);
            PU_SMART_CTOR(SideMenu)
            ~SideMenu();

            s32 GetX() override;
            s32 GetY() override;
            void SetX(s32 x); // Stubbed
            void SetY(s32 y);
            s32 GetWidth() override;
            s32 GetHeight() override;
            void OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y) override;
            void OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Touch) override;
            void SetOnItemSelected(std::function<void(u64, u32)> Fn);
            void SetOnSelectionChanged(std::function<void(u32)> Fn);
            void ClearItems();
            void AddItem(std::string Icon, std::string Text = "");
            void SetSuspendedItem(u32 Index);
            void UnsetSuspendedItem();
            void SetSelectedItem(u32 Index);
            void HandleMoveLeft();
            void HandleMoveRight();
            int GetSuspendedItem();
            u32 GetSelectedItem();
            u32 GetBaseItemIndex();
            void SetBaseItemIndex(u32 index);
            void SetBasePositions(u32 SelectedIdx, u32 BaseIdx);
            void ClearBorderIcons();
            void UpdateBorderIcons();
            void ResetMultiselections();
            void SetItemMultiselected(u32 index, bool selected);
            bool IsItemMultiselected(u32 index);
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
            std::vector<pu::ui::render::NativeTexture> ricons;
            std::vector<pu::ui::render::NativeTexture> ricons_texts;
            pu::ui::render::NativeTexture cursoricon;
            pu::ui::render::NativeTexture suspicon;
            pu::ui::render::NativeTexture leftbicon;
            pu::ui::render::NativeTexture rightbicon;
            pu::ui::render::NativeTexture mselicon;
            pu::ui::render::NativeFont textfont;
            std::chrono::steady_clock::time_point scrolltp;
            bool scrollmoveflag;
            std::chrono::steady_clock::time_point scrollmovetp;
            u32 scrollflag;
            u32 scrolltpvalue;
            u32 scrollcount;
            bool IsLeftFirst();
            bool IsRightLast();
            void ReloadIcons(u32 Direction);
    };
}