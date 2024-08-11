
#pragma once
#include <pu/Plutonium>
#include <ul/smi/smi_Protocol.hpp>

namespace ul::menu::ui {

    class IMenuLayout : public pu::ui::Layout {
        private:
            RecursiveMutex msg_queue_lock;
            std::queue<smi::MenuMessageContext> msg_queue;

            bool last_has_connection;
            u32 last_connection_strength;
            u32 last_battery_level;
            bool last_battery_is_charging;

        protected:
            void UpdateConnectionTopIcon(pu::ui::elm::Image::Ref &icon);
            void UpdateDateText(pu::ui::elm::TextBlock::Ref &text);
            void UpdateTimeText(pu::ui::elm::TextBlock::Ref &text);
            void UpdateBatteryTextAndTopIcons(pu::ui::elm::TextBlock::Ref &text, pu::ui::elm::Image::Ref &base_top_icon, pu::ui::elm::Image::Ref &charging_top_icon);

        public:
            IMenuLayout();

            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos);
            void NotifyMessageContext(const smi::MenuMessageContext &msg_ctx);
            virtual void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) = 0;
            virtual bool OnHomeButtonPress() = 0;
            virtual void DisposeAudio() = 0;
    };

}
