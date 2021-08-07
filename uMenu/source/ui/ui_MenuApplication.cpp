#include <ui/ui_MenuApplication.hpp>
#include <util/util_Misc.hpp>

extern ui::MenuApplication::Ref g_MenuApplication;
extern ui::TransitionGuard g_TransitionGuard;
extern u8 *g_ScreenCaptureBuffer;

extern cfg::Theme g_Theme;

extern JSON g_DefaultLanguage;
extern JSON g_MainLanguage;

namespace ui {

    std::string GetLanguageString(const std::string &name) {
        return cfg::GetLanguageString(g_MainLanguage, g_DefaultLanguage, name);
    }

    void UiOnHomeButtonDetection() {
        g_MenuApplication->GetLayout<IMenuLayout>()->DoOnHomeButtonPress();
    }

    void MenuApplication::OnLoad() {
        if(this->IsSuspended()) {
            bool flag;
            appletGetLastApplicationCaptureImageEx(g_ScreenCaptureBuffer, RawRGBAScreenBufferSize, &flag);
        }

        auto jbgm = JSON::object();
        util::LoadJSONFromFile(jbgm, cfg::GetAssetByTheme(g_Theme, "sound/BGM.json"));
        this->bgmjson = jbgm;
        this->bgm_loop = this->bgmjson.value("loop", true);
        this->bgm_fade_in_ms = this->bgmjson.value("fade_in_ms", 1500);
        this->bgm_fade_out_ms = this->bgmjson.value("fade_out_ms", 500);

        auto toasttextclr = pu::ui::Color::FromHex(GetUIConfigValue<std::string>("toast_text_color", "#e1e1e1ff"));
        auto toastbaseclr = pu::ui::Color::FromHex(GetUIConfigValue<std::string>("toast_base_color", "#282828ff"));
        this->notifToast = pu::ui::extras::Toast::New("...", "DefaultFont@20", toasttextclr, toastbaseclr);

        this->bgm = pu::audio::Open(cfg::GetAssetByTheme(g_Theme, "sound/BGM.mp3"));

        this->startupLayout = StartupLayout::New();
        this->menuLayout = MenuLayout::New(g_ScreenCaptureBuffer, this->uijson.value("suspended_final_alpha", 80));
        this->themeMenuLayout = ThemeMenuLayout::New();
        this->settingsMenuLayout = SettingsMenuLayout::New();
        this->languagesMenuLayout = LanguagesMenuLayout::New();

        switch(this->stmode) {
            case dmi::MenuStartMode::StartupScreen: {
                this->LoadStartupMenu();
                break;
            }
            default: {
                this->StartPlayBGM();
                this->LoadMenu();
                break;
            }
        }
    }

    void MenuApplication::SetInformation(dmi::MenuStartMode mode, dmi::DaemonStatus status, JSON ui_json) {
        this->stmode = mode;
        this->status = status;
        this->uijson = ui_json;
    }

    void MenuApplication::LoadMenu() {
        this->menuLayout->SetUser(this->status.selected_user);
        this->LoadLayout(this->menuLayout);
        this->loaded_menu = MenuType::Main;
    }

    void MenuApplication::LoadStartupMenu() {
        this->StopPlayBGM();
        this->startupLayout->ReloadMenu();
        this->LoadLayout(this->startupLayout);
        this->loaded_menu = MenuType::Startup;
    }

    void MenuApplication::LoadThemeMenu() {
        this->themeMenuLayout->Reload();
        this->LoadLayout(this->themeMenuLayout);
        this->loaded_menu = MenuType::Theme;
    }

    void MenuApplication::LoadSettingsMenu() {
        this->settingsMenuLayout->Reload();
        this->LoadLayout(this->settingsMenuLayout);
        this->loaded_menu = MenuType::Settings;
    }

    void MenuApplication::LoadSettingsLanguagesMenu() {
        this->languagesMenuLayout->Reload();
        this->LoadLayout(this->languagesMenuLayout);
        this->loaded_menu = MenuType::Languages;
    }

    bool MenuApplication::IsSuspended() {
        return this->IsTitleSuspended() || this->IsHomebrewSuspended();
    }

    bool MenuApplication::IsTitleSuspended() {
        return this->status.app_id > 0;
    }

    bool MenuApplication::IsHomebrewSuspended() {
        return strlen(this->status.params.nro_path);
    }

    bool MenuApplication::EqualsSuspendedHomebrewPath(const std::string &path) {
        return this->status.params.nro_path == path;
    }

    u64 MenuApplication::GetSuspendedApplicationId() {
        return this->status.app_id;
    }

    void MenuApplication::NotifyEndSuspended() {
        // Blanking the whole status would also blank the selected user...
        this->status.params = {};
        this->status.app_id = 0;
    }

    bool MenuApplication::LaunchFailed() {
        return this->stmode == dmi::MenuStartMode::MenuLaunchFailure;
    }

    void MenuApplication::ShowNotification(const std::string &text, u64 timeout) {
        this->EndOverlay();
        this->notifToast->SetText(text);
        this->StartOverlayWithTimeout(this->notifToast, timeout);
    }

    void MenuApplication::StartPlayBGM() {
        if(this->bgm != nullptr) {
            int loops = this->bgm_loop ? -1 : 1;
            if(this->bgm_fade_in_ms > 0) {
                pu::audio::PlayWithFadeIn(this->bgm, loops, this->bgm_fade_in_ms);
            }
            else {
                pu::audio::Play(this->bgm, loops);
            }
        }
    }

    void MenuApplication::StopPlayBGM() {
        if(this->bgm_fade_out_ms > 0) {
            pu::audio::FadeOut(this->bgm_fade_out_ms);
        }
        else {
            pu::audio::Stop();
        }
    }

    StartupLayout::Ref &MenuApplication::GetStartupLayout() {
        return this->startupLayout;
    }

    MenuLayout::Ref &MenuApplication::GetMenuLayout() {
        return this->menuLayout;
    }

    ThemeMenuLayout::Ref &MenuApplication::GetThemeMenuLayout() {
        return this->themeMenuLayout;
    }

    SettingsMenuLayout::Ref &MenuApplication::GetSettingsMenuLayout() {
        return this->settingsMenuLayout;
    }

    LanguagesMenuLayout::Ref &MenuApplication::GetLanguagesMenuLayout() {
        return this->languagesMenuLayout;
    }
    
    void MenuApplication::SetSelectedUser(AccountUid user_id) {
        this->status.selected_user = user_id;

        UL_ASSERT(dmi::menu::SendCommand(dmi::DaemonMessage::SetSelectedUser, [&](dmi::menu::MenuScopedStorageWriter &writer) {
            writer.Push(user_id);
            return ResultSuccess;
        },
        [&](dmi::menu::MenuScopedStorageReader &reader) {
            // ...
            return ResultSuccess;
        }));
    }

    AccountUid MenuApplication::GetSelectedUser() {
        return this->status.selected_user;
    }

    MenuType MenuApplication::GetCurrentLoadedMenu() {
        return this->loaded_menu;
    }

}