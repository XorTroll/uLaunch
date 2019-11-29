
#pragma once
#include <q_Include.hpp>
#include <pu/Plutonium>

namespace ui
{
    enum class QuickMenuDirection
    {
        Up,
        Down,
        Left,
        Right,
        UpLeft,
        UpRight,
        DownLeft,
        DownRight,
        None
    };

    struct QuickMenuSubItem
    {
        std::function<void()> on_select;
        pu::ui::render::NativeTexture nicon;
    };

    class QuickMenu : public pu::ui::elm::Element
    {
        static constexpr s32 MainItemSize = 300;
        static constexpr s32 SubItemsSize = 150;
        static constexpr s32 CommonAreaSize = 50;

        static constexpr s32 MainItemX = (1280 - MainItemSize) / 2;
        static constexpr s32 MainItemY = (720 - MainItemSize) / 2;

        public:
            QuickMenu(std::string main_icon);
            PU_SMART_CTOR(QuickMenu)
            ~QuickMenu();

            void SetEntry(QuickMenuDirection direction, std::string icon, std::function<void()> on_selected);
            void RemoveEntry(QuickMenuDirection direction);

            s32 GetX();
            s32 GetY();
            s32 GetWidth();
            s32 GetHeight();
            
            void Toggle(); // Off if on, on if off (just change to the opposite state)
            bool IsOn();

            void OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y);
            void OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos);

        private:
            QuickMenuDirection GetCurrentDirection();
            std::tuple<s32, s32> ComputePositionForDirection(QuickMenuDirection direction);
            bool on;
            s64 off_wait;
            pu::ui::render::NativeTexture nmain;
            u64 lastheld;
            s32 bgalpha;
            s32 fgalpha;
            std::map<QuickMenuDirection, QuickMenuSubItem> item_map;
    };
}