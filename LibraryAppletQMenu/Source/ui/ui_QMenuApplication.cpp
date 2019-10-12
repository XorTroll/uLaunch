#include <ui/ui_QMenuApplication.hpp>

extern u8 *app_buf;

namespace ui
{
    void QMenuApplication::OnLoad()
    {
        pu::ui::render::SetDefaultFont("romfs:/default/ui/Font.ttf");

        this->startupLayout = StartupLayout::New(pu::ui::Color(10, 120, 255, 255));
        this->menuLayout = MenuLayout::New(app_buf);
        this->tsuspended = false;

        switch(this->stmode)
        {
            case am::QMenuStartMode::MenuNormal:
                this->LoadMenu();
                break;
            case am::QMenuStartMode::MenuApplicationSuspended:
                this->SetTitleSuspended(true);
                this->LoadMenu();
                break;
            case am::QMenuStartMode::MenuLaunchFailure:
                this->LoadMenu();
                break;
            default:
                this->LoadLayout(this->startupLayout);
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

    bool QMenuApplication::LaunchFailed()
    {
        return (this->stmode == am::QMenuStartMode::MenuLaunchFailure);
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
        this->SetTitleSuspended(app_id != 0);
        this->suspended_appid = app_id;
    }

    u64 QMenuApplication::GetSuspendedApplicationId()
    {
        return this->suspended_appid;
    }
}