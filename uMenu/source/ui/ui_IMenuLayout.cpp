#include <ui/ui_IMenuLayout.hpp>
#include <ui/ui_Actions.hpp>

namespace ui
{
    IMenuLayout::IMenuLayout() : home_press_lock(EmptyMutex), home_pressed(false)
    {
        this->SetOnInput(std::bind(&IMenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void IMenuLayout::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos)
    {
        mutexLock(&this->home_press_lock);
        auto home_press = this->home_pressed;
        this->home_pressed = false;
        mutexUnlock(&this->home_press_lock);
        if(home_press) this->OnHomeButtonPress();
        if(!hidIsControllerConnected(CONTROLLER_HANDHELD) && !hidIsControllerConnected(CONTROLLER_PLAYER_1)) actions::ShowControllerSupport();
        this->OnMenuInput(down, up, held, pos);
    }

    void IMenuLayout::DoOnHomeButtonPress()
    {
        mutexLock(&this->home_press_lock);
        this->home_pressed = true;
        mutexUnlock(&this->home_press_lock);
    }
}