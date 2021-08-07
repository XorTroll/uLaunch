#include <ui/ui_IMenuLayout.hpp>
#include <ui/ui_Actions.hpp>

namespace ui {

    IMenuLayout::IMenuLayout() : home_pressed(false) {
        this->SetOnInput(std::bind(&IMenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void IMenuLayout::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) {
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

        this->OnMenuInput(down, up, held, touch_pos);
    }

    void IMenuLayout::DoOnHomeButtonPress() {
        this->home_pressed = true;
    }

}