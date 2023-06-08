
#pragma once
#include <pu/Plutonium>

namespace ul::menu::ui {

    class IMenuLayout : public pu::ui::Layout {
        private:
            std::atomic_bool home_pressed;

        public:
            IMenuLayout();

            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos);
            void DoOnHomeButtonPress();
            virtual void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) = 0;
            virtual bool OnHomeButtonPress() = 0;
    };

}