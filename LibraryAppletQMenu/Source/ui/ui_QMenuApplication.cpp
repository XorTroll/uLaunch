#include <ui/ui_QMenuApplication.hpp>
#include <util/util_JSON.hpp>

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

        auto [_rc, jui] = util::LoadJSONFromFile(cfg::ProcessedThemeResource(theme, "ui/UI.json"));
        this->uijson = jui;
        auto [_rc2, jbgm] = util::LoadJSONFromFile(cfg::ProcessedThemeResource(theme, "sound/BGM.json"));
        this->bgmjson = jbgm;

        this->bgm = pu::audio::Open(cfg::ProcessedThemeResource(theme, "sound/BGM.mp3"));

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
        this->StartPlayBGM();
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
        bool loop = this->bgmjson.value("loop", true);
        int fade_milli = this->bgmjson.value("fade_in_ms", 1500);
        if(this->bgm != NULL)
        {
            int loops = loop ? -1 : 1;
            if(fade_milli > 0) pu::audio::PlayWithFadeIn(this->bgm, loops, fade_milli);
            else pu::audio::Play(this->bgm, loops);
        }
    }

    void QMenuApplication::StopPlayBGM()
    {
        int fade_milli = this->bgmjson.value("fade_out_ms", 500);
        if(fade_milli > 0) pu::audio::FadeOut(fade_milli);
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