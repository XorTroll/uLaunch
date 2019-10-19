#include <ui/ui_QMenuApplication.hpp>

extern u8 *app_buf;
extern cfg::ProcessedTheme theme;

namespace ui
{
    void QMenuApplication::OnLoad()
    {
        pu::ui::render::SetDefaultFont(cfg::ProcessedThemeResource(theme, "ui/Font.ttf"));

        am::QMenuCommandWriter writer(am::QDaemonMessage::GetSuspendedInfo);
        writer.FinishWrite();
        am::QMenuCommandResultReader reader;
        if(reader) this->suspinfo = reader.Read<am::QSuspendedInfo>();
        reader.FinishRead();

        if(this->IsSuspended())
        {
            bool flag;
            appletGetLastApplicationCaptureImageEx(app_buf, 1280 * 720 * 4, &flag);
        }

        this->startupLayout = StartupLayout::New();
        bool hb = false;
        if(this->stmode == am::QMenuStartMode::MenuHomebrewMode) hb = true;
        this->menuLayout = MenuLayout::New(app_buf, 80, hb);

        switch(this->stmode)
        {
            case am::QMenuStartMode::MenuNormal:
            case am::QMenuStartMode::MenuHomebrewMode:
            case am::QMenuStartMode::MenuApplicationSuspended:
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

    bool QMenuApplication::IsSuspended()
    {
        return (this->IsTitleSuspended() || this->IsHomebrewSuspended());
    }

    bool QMenuApplication::IsTitleSuspended()
    {
        return (this->suspinfo.app_id != 0);
    }

    bool QMenuApplication::IsHomebrewSuspended()
    {
        return strlen(this->suspinfo.input.nro_path);
    }

    std::string QMenuApplication::GetSuspendedHomebrewPath()
    {
        return this->suspinfo.input.nro_path;
    }

    u64 QMenuApplication::GetSuspendedApplicationId()
    {
        return this->suspinfo.app_id;
    }

    void QMenuApplication::NotifyEndSuspended()
    {
        this->suspinfo = {};
    }

    bool QMenuApplication::LaunchFailed()
    {
        return (this->stmode == am::QMenuStartMode::MenuLaunchFailure);
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
}