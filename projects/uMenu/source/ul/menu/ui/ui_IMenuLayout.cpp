#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/menu/ui/ui_Actions.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    IMenuLayout::IMenuLayout() : msg_queue_lock(), msg_queue() {
        this->SetOnInput(std::bind(&IMenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void IMenuLayout::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        {
            std::scoped_lock lk(this->msg_queue_lock);

            if(!this->msg_queue.empty()) {
                const auto first_msg = this->msg_queue.front();

                switch(first_msg.msg) {
                    case smi::MenuMessage::HomeRequest: {
                        if(this->OnHomeButtonPress()) {
                            this->msg_queue.pop();
                        }
                        break;
                    }
                    case smi::MenuMessage::GameCardMountFailure: {
                        g_MenuApplication->CreateShowDialog("GC mount", "GC mount failed: " + util::FormatResultHex(first_msg.gc_mount_failure.mount_rc), { "K" }, true);
                        this->msg_queue.pop();
                        break;
                    }
                    case smi::MenuMessage::SdCardEjected: {
                        g_MenuApplication->CreateShowDialog("SD ejected", "SD ejected", { "K" }, true);
                        this->msg_queue.pop();

                        RebootSystem();
                        break;
                    }
                    default: {
                        this->msg_queue.pop();
                        break;
                    }
                }
            }
        }

        /* TODONEW
        if(!hidIsControllerConnected(CONTROLLER_HANDHELD) && !hidIsControllerConnected(CONTROLLER_PLAYER_1)) {
            ShowControllerSupport();
        }
        */

        this->OnMenuInput(keys_down, keys_up, keys_held, touch_pos);
    }

    void IMenuLayout::NotifyMessageContext(const smi::MenuMessageContext msg_ctx) {
        std::scoped_lock lk(this->msg_queue_lock);
        // TODONEW: remove consequent homemenu requests?
        this->msg_queue.push(msg_ctx);
    }

}