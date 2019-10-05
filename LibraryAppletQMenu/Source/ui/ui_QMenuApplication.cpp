#include <ui/ui_QMenuApplication.hpp>

extern u8 *app_buf;

namespace ui
{
    void QMenuApplication::OnLoad()
    {
        pu::ui::render::SetDefaultFont("romfs:/Gilroy-Bold.ttf");

        this->startupLayout = StartupLayout::New(pu::ui::Color(10, 120, 255, 255));
        this->menuLayout = MenuLayout::New(app_buf);
        this->tsuspended = false;

        switch(this->stmode)
        {
            case am::QMenuStartMode::StartupScreen:
                this->LoadLayout(this->startupLayout);
                break;
            case am::QMenuStartMode::MenuNormal:
                this->LoadMenu();
                break;
            case am::QMenuStartMode::MenuApplicationSuspended:
                this->LoadMenu();
                this->SetTitleSuspended(true);
                break;
        }
    }

    void QMenuApplication::SetStartMode(am::QMenuStartMode mode)
    {
        this->stmode = mode;
    }

    void QMenuApplication::LoadMenu()
    {
        this->LoadLayout(this->menuLayout);
    }

    bool QMenuApplication::IsTitleSuspended()
    {
        return this->tsuspended;
    }

    void QMenuApplication::SetTitleSuspended(bool suspended)
    {
        this->tsuspended = suspended;
    }
    
    void QMenuApplication::SetSelectedUser(u128 user_id)
    {
        am::QMenuCommandWriter writer(am::QDaemonMessage::SetSelectedUser);
        writer.Write<u128>(user_id);
        writer.FinishWrite();

        this->selected_user = user_id;
    }

    u128 QMenuApplication::GetSelectedUser()
    {
        return this->selected_user;
    }

    void QMenuApplication::SetSuspendedApplicationId(u64 app_id)
    {
        this->SetTitleSuspended(true);
        this->suspended_appid = app_id;
    }

    u64 QMenuApplication::GetSuspendedApplicationId()
    {
        return this->suspended_appid;
    }
}