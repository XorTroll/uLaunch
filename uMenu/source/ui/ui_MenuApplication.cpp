#include <ui/ui_MenuApplication.hpp>
#include <util/util_Misc.hpp>

extern u8 *app_buf;
extern cfg::Theme theme;

namespace ui
{
    MenuApplication::~MenuApplication()
    {
        pu::audio::Delete(this->bgm);
    }

    void MenuApplication::OnLoad()
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
            case am::MenuStartMode::StartupScreen:
                this->LoadStartupMenu();
                break;
            default:
                this->StartPlayBGM();
                this->LoadMenu();
                break;
        }
    }

    void MenuApplication::SetInformation(am::MenuStartMode mode, am::DaemonStatus status)
    {
        this->stmode = mode;
        this->status = status;
    }

    void MenuApplication::LoadMenu()
    {
        this->menuLayout->SetUser(this->status.selected_user);
        this->LoadLayout(this->menuLayout);
    }

    void MenuApplication::LoadStartupMenu()
    {
        this->StopPlayBGM();
        this->startupLayout->ReloadMenu();
        this->LoadLayout(this->startupLayout);
    }

    void MenuApplication::LoadThemeMenu()
    {
        this->themeMenuLayout->Reload();
        this->LoadLayout(this->themeMenuLayout);
    }

    void MenuApplication::LoadSettingsMenu()
    {
        this->settingsMenuLayout->Reload();
        this->LoadLayout(this->settingsMenuLayout);
    }

    void MenuApplication::LoadSettingsLanguagesMenu()
    {
        this->languagesMenuLayout->Reload();
        this->LoadLayout(this->languagesMenuLayout);
    }

    bool MenuApplication::IsSuspended()
    {
        return this->IsTitleSuspended() || this->IsHomebrewSuspended();
    }

    bool MenuApplication::IsTitleSuspended()
    {
        return this->status.app_id > 0;
    }

    bool MenuApplication::IsHomebrewSuspended()
    {
        return strlen(this->status.params.nro_path);
    }

    bool MenuApplication::EqualsSuspendedHomebrewPath(std::string path)
    {
        return this->status.params.nro_path == path;
    }

    u64 MenuApplication::GetSuspendedApplicationId()
    {
        return this->status.app_id;
    }

    void MenuApplication::NotifyEndSuspended()
    {
        // Blanking the whole status would also blank the selected user...
        this->status.params = {};
        this->status.app_id = 0;
    }

    bool MenuApplication::LaunchFailed()
    {
        return (this->stmode == am::MenuStartMode::MenuLaunchFailure);
    }

    void MenuApplication::ShowNotification(std::string text, u64 timeout)
    {
        this->EndOverlay();
        this->notifToast->SetText(text);
        this->StartOverlayWithTimeout(this->notifToast, timeout);
    }

    void MenuApplication::StartPlayBGM()
    {
        if(this->bgm != NULL)
        {
            int loops = this->bgm_loop ? -1 : 1;
            if(this->bgm_fade_in_ms > 0) pu::audio::PlayWithFadeIn(this->bgm, loops, this->bgm_fade_in_ms);
            else pu::audio::Play(this->bgm, loops);
        }
    }

    void MenuApplication::StopPlayBGM()
    {
        if(this->bgm_fade_out_ms > 0) pu::audio::FadeOut(this->bgm_fade_out_ms);
        else pu::audio::Stop();
    }
    
    void MenuApplication::SetSelectedUser(AccountUid user_id)
    {
        am::MenuCommandWriter writer(am::DaemonMessage::SetSelectedUser);
        writer.Write<AccountUid>(user_id);
        writer.FinishWrite();

        memcpy(&this->status.selected_user, &user_id, sizeof(user_id));
    }

    AccountUid MenuApplication::GetSelectedUser()
    {
        return this->status.selected_user;
    }
}