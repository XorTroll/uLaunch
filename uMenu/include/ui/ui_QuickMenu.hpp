
#pragma once
#include <am/am_DaemonMessages.hpp>
#include <pu/Plutonium>

namespace ui {

    enum class QuickMenuDirection {
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

    struct QuickMenuSubItem {
        std::function<void()> on_select;
        pu::sdl2::Texture nicon;
    };

    void QuickMenuOnHomeButtonDetection();

    class QuickMenu : public pu::ui::elm::Element {

        public:
            static constexpr s32 MainItemSize = 300;
            static constexpr s32 SubItemsSize = 150;
            static constexpr s32 CommonAreaSize = 50;

            static constexpr s32 MainItemX = (1280 - MainItemSize) / 2;
            static constexpr s32 MainItemY = (720 - MainItemSize) / 2;

        private:
            bool on;
            s32 bgalpha;
            pu::ui::elm::Menu::Ref options_menu;

        public:
            QuickMenu(const std::string &main_icon);
            PU_SMART_CTOR(QuickMenu)

            static inline void RegisterHomeButtonDetection() {
                am::RegisterOnMessageDetect(&QuickMenuOnHomeButtonDetection, dmi::MenuMessage::HomeRequest);
            }

            s32 GetX();
            s32 GetY();
            s32 GetWidth();
            s32 GetHeight();
            
            void Toggle(); // Off if on, on if off (just change to the opposite state)
            bool IsOn();

            void OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y);
            void OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos);

    };

}