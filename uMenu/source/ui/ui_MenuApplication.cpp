#include <ui/ui_MenuApplication.hpp>
#include <util/util_Misc.hpp>

extern ui::MenuApplication::Ref g_MenuApplication;
extern ui::TransitionGuard g_TransitionGuard;

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
        u8 *screen_capture_buf = nullptr;
        if(this->IsSuspended()) {
            screen_capture_buf = new u8[PlainRgbaScreenBufferSize]();
            bool flag;
            appletGetLastApplicationCaptureImageEx(screen_capture_buf, PlainRgbaScreenBufferSize, &flag);
        }

        this->bgm_json = JSON::object();
        util::LoadJSONFromFile(this->bgm_json, cfg::GetAssetByTheme(g_Theme, "sound/BGM.json"));
        this->bgm_loop = this->bgm_json.value("loop", true);
        this->bgm_fade_in_ms = this->bgm_json.value("fade_in_ms", 1500);
        this->bgm_fade_out_ms = this->bgm_json.value("fade_out_ms", 500);

        const auto toast_text_clr = pu::ui::Color::FromHex(GetUIConfigValue<std::string>("toast_text_color", "#e1e1e1ff"));
        const auto toast_base_clr = pu::ui::Color::FromHex(GetUIConfigValue<std::string>("toast_base_color", "#282828ff"));
        this->notif_toast = pu::ui::extras::Toast::New("...", pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium), toast_text_clr, toast_base_clr);

        this->bgm = pu::audio::OpenMusic(cfg::GetAssetByTheme(g_Theme, "sound/BGM.mp3"));

        this->text_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        this->menu_focus_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        this->menu_bg_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        const u8 suspended_final_alpha = this->ui_json.value("suspended_final_alpha", 80);
        this->startup_lyt = StartupLayout::New();
        this->menu_lyt = MenuLayout::New(screen_capture_buf, suspended_final_alpha);
        this->theme_menu_lyt = ThemeMenuLayout::New();
        this->settings_menu_lyt = SettingsMenuLayout::New();
        this->languages_menu_lyt = LanguagesMenuLayout::New();

        this->logout_sfx=pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/Logout.wav"));

        switch(this->start_mode) {
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

    void MenuApplication::PlayLogoutSfx(){
        pu::audio::PlaySfx(this->logout_sfx);
    }

    void MenuApplication::ShowNotification(const std::string &text, const u64 timeout) {
        this->EndOverlay();
        this->notif_toast->SetText(text);
        this->StartOverlayWithTimeout(this->notif_toast, timeout);
    }

    void MenuApplication::StartPlayBGM() {
        if(this->bgm != nullptr) {
            const int loops = this->bgm_loop ? -1 : 1;
            if(this->bgm_fade_in_ms > 0) {
                pu::audio::PlayMusicWithFadeIn(this->bgm, loops, this->bgm_fade_in_ms);
            }
            else {
                pu::audio::PlayMusic(this->bgm, loops);
            }
        }
    }

    void MenuApplication::StopPlayBGM() {
        if(this->bgm_fade_out_ms > 0) {
            pu::audio::FadeOutMusic(this->bgm_fade_out_ms);
        }
        else {
            pu::audio::StopMusic();
        }
    }
    
    void MenuApplication::SetSelectedUser(const AccountUid user_id) {
        this->daemon_status.selected_user = user_id;

        UL_RC_ASSERT(dmi::menu::SendCommand(dmi::DaemonMessage::SetSelectedUser, [&](dmi::menu::MenuScopedStorageWriter &writer) {
            writer.Push(user_id);
            return ResultSuccess;
        },
        [&](dmi::menu::MenuScopedStorageReader &reader) {
            // ...
            return ResultSuccess;
        }));
    }

}