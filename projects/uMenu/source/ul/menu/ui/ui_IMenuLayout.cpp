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
            ScopedLock lk(this->msg_queue_lock);

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
                        // TODO: move somewhere else?
                        g_MenuApplication->DisplayDialog(GetLanguageString("gamecard"), GetLanguageString("gamecard_mount_failed") + " " + util::FormatResultHex(first_msg.gc_mount_failure.mount_rc), { GetLanguageString("ok") }, true);
                        this->msg_queue.pop();
                        break;
                    }
                    case smi::MenuMessage::SdCardEjected: {
                        this->msg_queue.pop();

                        while(true) {
                            const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("sd_card"), GetLanguageString("sd_card_ejected"), { GetLanguageString("shutdown"), GetLanguageString("reboot") }, false);
                            if(option == 0) {
                                ShutdownSystem();
                            }
                            else if(option == 1) {
                                RebootSystem();
                            }
                        }
                        break;
                    }
                    case smi::MenuMessage::PreviousLaunchFailure: {
                        g_MenuApplication->NotifyLaunchFailed();
                        this->msg_queue.pop();

                        break;
                    }
                    case smi::MenuMessage::ChosenHomebrew: {
                        g_MenuApplication->NotifyHomebrewChosen(first_msg.chosen_hb.nro_path);
                        this->msg_queue.pop();

                        break;
                    }
                    default: {
                        this->msg_queue.pop();
                        break;
                    }
                }
            }
        }

        /* TODO (new)
        if(!hidIsControllerConnected(CONTROLLER_HANDHELD) && !hidIsControllerConnected(CONTROLLER_PLAYER_1)) {
            ShowControllerSupport();
        }
        */

        this->OnMenuInput(keys_down, keys_up, keys_held, touch_pos);
    }

    void IMenuLayout::NotifyMessageContext(const smi::MenuMessageContext msg_ctx) {
        ScopedLock lk(this->msg_queue_lock);

        // Remove consequent homemenu requests
        if(msg_ctx.msg == smi::MenuMessage::HomeRequest) {
            if(!this->msg_queue.empty()) {
                if(this->msg_queue.front().msg == smi::MenuMessage::HomeRequest) {
                    return;
                }
            }
        }

        this->msg_queue.push(msg_ctx);
    }

}