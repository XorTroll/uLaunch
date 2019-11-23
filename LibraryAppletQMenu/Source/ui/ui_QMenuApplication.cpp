#include <ui/ui_QMenuApplication.hpp>
#include <util/util_Misc.hpp>

extern u8 *app_buf;
extern cfg::Theme theme;

namespace ui
{
    QMenuApplication::~QMenuApplication()
    {
        pu::audio::Delete(this->bgm);
    }

    void QMenuApplication::OnLoad()
    {
        pu::ui::render::SetDefaultFont(cfg::GetAssetByTheme(theme, "ui/Font.ttf"));

        if(this->IsSuspended())
        {
            bool flag;
            appletGetLastApplicationCaptureImageEx(app_buf, RawRGBAScreenBufferSize, &flag);
        }

        auto [_rc, jui] = util::LoadJSONFromFile(cfg::GetAssetByTheme(theme, "ui/UI.json"));
        this->uijson = jui;
        auto [_rc2, jbgm] = util::LoadJSONFromFile(cfg::GetAssetByTheme(theme, "sound/BGM.json"));
        this->bgmjson = jbgm;
        this->bgm_loop = this->bgmjson.value("loop", true);
        this->bgm_fade_in_ms = this->bgmjson.value("fade_in_ms", 1500);
        this->bgm_fade_out_ms = this->bgmjson.value("fade_out_ms", 500);

        pu::ui::Color toasttextclr = pu::ui::Color::FromHex(GetUIConfigValue<std::string>("toast_text_color", "#e1e1e1ff"));
        pu::ui::Color toastbaseclr = pu::ui::Color::FromHex(GetUIConfigValue<std::string>("toast_base_color", "#282828ff"));
        this->notifToast = pu::ui::extras::Toast::New("...", 20, toasttextclr, toastbaseclr);

        this->bgm = pu::audio::Open(cfg::GetAssetByTheme(theme, "sound/BGM.mp3"));

        this->startupLayout = StartupLayout::New();
        this->menuLayout = MenuLayout::New(app_buf, this->uijson.value("suspended_final_alpha", 80));
        this->themeMenuLayout = ThemeMenuLayout::New();
        this->settingsMenuLayout = SettingsMenuLayout::New();
        this->languagesMenuLayout = LanguagesMenuLayout::New();

        switch(this->stmode)
        {
            case am::QMenuStartMode::StartupScreen:
                this->LoadStartupMenu();
                break;
            default:
                this->StartPlayBGM();
                this->LoadMenu();
                break;
        }
    }

    void QMenuApplication::SetInformation(am::QMenuStartMode mode, am::QDaemonStatus status)
    {
        this->stmode = mode;
        memcpy(&this->status, &status, sizeof(status));
    }

    void QMenuApplication::LoadMenu()
    {
        this->menuLayout->SetUser(this->status.selected_user);
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

    void QMenuApplication::LoadSettingsMenu()
    {
        this->settingsMenuLayout->Reload();
        this->LoadLayout(this->settingsMenuLayout);
    }

    void QMenuApplication::LoadSettingsLanguagesMenu()
    {
        this->languagesMenuLayout->Reload();
        this->LoadLayout(this->languagesMenuLayout);
    }

    bool QMenuApplication::IsSuspended()
    {
        return (this->IsTitleSuspended() || this->IsHomebrewSuspended());
    }

    bool QMenuApplication::IsTitleSuspended()
    {
        return (this->status.app_id > 0);
    }

    bool QMenuApplication::IsHomebrewSuspended()
    {
        return strlen(this->status.input.nro_path);
    }

    bool QMenuApplication::EqualsSuspendedHomebrewPath(std::string path)
    {
        return (strcasecmp(this->status.input.nro_path, path.c_str()) == 0);
    }

    u64 QMenuApplication::GetSuspendedApplicationId()
    {
        return this->status.app_id;
    }

    void QMenuApplication::NotifyEndSuspended()
    {
        // Blanking the whole status would also blank the selected user...
        this->status.input = {};
        this->status.app_id = 0;
    }

    bool QMenuApplication::LaunchFailed()
    {
        return (this->stmode == am::QMenuStartMode::MenuLaunchFailure);
    }

    void QMenuApplication::ShowNotification(std::string text, u64 timeout)
    {
        this->EndOverlay();
        this->notifToast->SetText(text);
        this->StartOverlayWithTimeout(this->notifToast, timeout);
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

        memcpy(&this->status.selected_user, &user_id, sizeof(user_id));
    }

    u128 QMenuApplication::GetSelectedUser()
    {
        return this->status.selected_user;
    }
}