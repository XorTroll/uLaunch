#include <ul/menu/ui/menu/menu_MenuLayout.hpp>

namespace ul::menu::ui::menu {

    MenuLayout::MenuLayout() : home_pressed(false) {
        this->SetOnInput(std::bind(&MenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void MenuLayout::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
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

    void MenuLayout::DoOnHomeButtonPress() {
        // Handle the press on main thread since this handler will get called from a different thread
        this->home_pressed = true;
    }

}