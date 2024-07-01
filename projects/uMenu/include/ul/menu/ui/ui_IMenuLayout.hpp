
#pragma once
#include <pu/Plutonium>
#include <ul/smi/smi_Protocol.hpp>

namespace ul::menu::ui {

    class IMenuLayout : public pu::ui::Layout {
        private:
            RecursiveMutex msg_queue_lock;
            std::queue<smi::MenuMessageContext> msg_queue;

        public:
            IMenuLayout();

            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos);
            void NotifyMessageContext(const smi::MenuMessageContext msg_ctx);
            virtual void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) = 0;
            virtual bool OnHomeButtonPress() = 0;
    };

}
