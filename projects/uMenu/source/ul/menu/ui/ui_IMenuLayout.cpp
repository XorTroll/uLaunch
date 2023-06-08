#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/menu/ui/ui_Actions.hpp>

namespace ul::menu::ui {

    IMenuLayout::IMenuLayout() : home_pressed(false) {
        this->SetOnInput(std::bind(&IMenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void IMenuLayout::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->home_pressed) {
            if(this->OnHomeButtonPress()) {
                // Input consumed
                this->home_pressed = false;
            }
        }

        /* TODO
        if(!hidIsControllerConnected(CONTROLLER_HANDHELD) && !hidIsControllerConnected(CONTROLLER_PLAYER_1)) {
            actions::ShowControllerSupport();
        }
        */

        this->OnMenuInput(keys_down, keys_up, keys_held, touch_pos);
    }

    void IMenuLayout::DoOnHomeButtonPress() {
        this->home_pressed = true;
    }

}