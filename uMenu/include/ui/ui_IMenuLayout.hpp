
#pragma once
#include <pu/Plutonium>
#include <ul_Include.hpp>

namespace ui
{
    class IMenuLayout : public pu::ui::Layout
    {
        private:
            Mutex home_press_lock;
            bool home_pressed;

        public:
            IMenuLayout();

            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos);
            virtual void OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch pos) = 0;
            void DoOnHomeButtonPress();
            virtual void OnHomeButtonPress() = 0;
    };
}