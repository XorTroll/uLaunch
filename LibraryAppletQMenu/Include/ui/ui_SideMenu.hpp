
#pragma once
#include <pu/Plutonium>

namespace ui
{
    class SideMenu : public pu::ui::elm::Element
    {
        static constexpr u32 ItemSize = 256;
        static constexpr u32 Margin = 20;
        static constexpr u32 ItemCount = 4;
        static constexpr u32 FocusSize = 15;
        static constexpr u32 FocusMargin = 5;

        public:
            SideMenu(pu::ui::Color FocusColor, pu::ui::Color SuspendedColor);
            PU_SMART_CTOR(SideMenu)

            s32 GetX() override;
            s32 GetY() override;
            s32 GetWidth() override;
            s32 GetHeight() override;
            void OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y) override;
            void OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Touch) override;
            void SetOnItemSelected(std::function<void(u32)> Fn);
            void SetOnItemFocus(std::function<void(u32)> Fn);
            void ClearItems();
            void AddItem(std::string Icon);
            void SetSuspendedItem(u32 Index);
            void UnsetSuspendedItem();
            void SetSelectedItem(u32 Index);
            void HandleMoveLeft();
            void HandleMoveRight();
            int GetSuspendedItem();
            u32 GetSelectedItem();
        private:
            void UpdateBorderIcons();
            s32 x;
            u32 selitm;
            u32 preselitm;
            int suspitm;
            u32 baseiconidx;
            u8 movalpha;
            pu::ui::Color selclr;
            pu::ui::Color suspclr;
            std::vector<std::string> icons;
            std::function<void(u32)> onfocus;
            std::function<void(u32)> onclick;
            std::vector<pu::ui::render::NativeTexture> ricons;
            pu::ui::render::NativeTexture leftbicon;
            pu::ui::render::NativeTexture rightbicon;
            bool IsLeftFirst();
            bool IsRightLast();
            void ReloadIcons(u32 Direction);
    };
}