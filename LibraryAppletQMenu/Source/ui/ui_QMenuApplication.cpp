#include <ui/ui_QMenuApplication.hpp>
#include <util/util_Misc.hpp>

extern u8 *app_buf;
extern cfg::ProcessedTheme theme;

namespace ui
{
    QMenuApplication::~QMenuApplication()
    {
        pu::audio::Delete(this->bgm);
    }

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
            appletGetLastApplicationCaptureImageEx(app_buf, RawRGBAScreenBufferSize, &flag);
        }

        auto [_rc, jui] = util::LoadJSONFromFile(cfg::ProcessedThemeResource(theme, "ui/UI.json"));
        this->uijson = jui;
        auto [_rc2, jbgm] = util::LoadJSONFromFile(cfg::ProcessedThemeResource(theme, "sound/BGM.json"));
        this->bgmjson = jbgm;
        this->bgm_loop = this->bgmjson.value("loop", true);
        this->bgm_fade_in_ms = this->bgmjson.value("fade_in_ms", 1500);
        this->bgm_fade_out_ms = this->bgmjson.value("fade_out_ms", 500);

        this->bgm = pu::audio::Open(cfg::ProcessedThemeResource(theme, "sound/BGM.mp3"));

        this->startupLayout = StartupLayout::New();
        this->menuLayout = MenuLayout::New(app_buf, 80);
        this->themeMenuLayout = ThemeMenuLayout::New();

        switch(this->stmode)
        {
            case am::QMenuStartMode::Menu:
            case am::QMenuStartMode::MenuApplicationSuspended:
            case am::QMenuStartMode::MenuLaunchFailure:
            {
                // Returned from applet/title, QDaemon has the user but we don't, so...
                am::QMenuCommandWriter writer(am::QDaemonMessage::GetSelectedUser);
                writer.FinishWrite();
                am::QMenuCommandResultReader res;
                this->selected_user = res.Read<u128>();
                res.FinishRead();

                this->LoadMenu();
                break;
            }
            default:
                this->LoadStartupMenu();
                break;
        }
    }

    void QMenuApplication::SetStartMode(am::QMenuStartMode mode)
    {
        this->stmode = mode;
    }

    void QMenuApplication::LoadMenu()
    {
        this->menuLayout->SetUser(this->selected_user);
        this->LoadLayout(this->menuLayout);
    }

    void QMenuApplication::LoadStartupMenu()
    {
        this->StopPlayBGM();
        this->startupLayout->ReloadMenu();
        this->LoadLayout(this->startupLayout);
    }

    void QMenuApplication::LoadThemeMenu()
    {
        this->themeMenuLayout->Reload();
        this->LoadLayout(this->themeMenuLayout);
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

    void QMenuApplication::StartPlayBGM()
    {
        if(this->bgm != NULL)
        {
            int loops = this->bgm_loop ? -1 : 1;
            if(this->bgm_fade_in_ms > 0) pu::audio::PlayWithFadeIn(this->bgm, loops, this->bgm_fade_in_ms);
            else pu::audio::Play(this->bgm, loops);
        }
    }

    void QMenuApplication::StopPlayBGM()
    {
        if(this->bgm_fade_out_ms > 0) pu::audio::FadeOut(this->bgm_fade_out_ms);
        else pu::audio::Stop();
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