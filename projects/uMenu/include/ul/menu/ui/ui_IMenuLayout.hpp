
#pragma once
#include <pu/Plutonium>
#include <ul/smi/smi_Protocol.hpp>
#include <ul/os/os_System.hpp>
#include <ul/menu/ui/ui_MultiTextBlock.hpp>

namespace ul::menu::ui {

    class IMenuLayout : public pu::ui::Layout {
        public:
            static constexpr u32 TimeDotsAnimStepCount = 60;

        private:
            RecursiveMutex msg_queue_lock;
            std::queue<smi::MenuMessageContext> msg_queue;

            bool last_has_connection;
            u32 last_connection_strength;

            u32 last_battery_level;
            bool last_battery_is_charging;

            os::Time last_time;
            os::Date last_date;

            u32 time_anim_frame;
            bool time_anim_dots;

        protected:
            void UpdateConnectionTopIcon(pu::ui::elm::Image::Ref &icon);
            
            void UpdateDateText(pu::ui::elm::TextBlock::Ref &date_text);

            void InitializeTimeText(MultiTextBlock::Ref &time_mtext, const std::string &ui_menu, const std::string &ui_name);
            void UpdateTimeText(MultiTextBlock::Ref &time_mtext);
            
            void UpdateBatteryTextAndTopIcons(pu::ui::elm::TextBlock::Ref &text, pu::ui::elm::Image::Ref &base_top_icon, pu::ui::elm::Image::Ref &charging_top_icon);

        public:
            IMenuLayout();

            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos);
            void NotifyMessageContext(const smi::MenuMessageContext &msg_ctx);
            
            virtual void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) = 0;
            
            virtual bool OnHomeButtonPress() = 0;
            
            virtual void LoadSfx() = 0;
            virtual void DisposeSfx() = 0;
    };

}
