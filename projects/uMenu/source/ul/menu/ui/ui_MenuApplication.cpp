#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::menu::ui::TransitionGuard g_TransitionGuard;

extern ul::cfg::Theme g_Theme;

extern ul::util::JSON g_DefaultLanguage;
extern ul::util::JSON g_MainLanguage;

namespace ul::menu::ui {

    namespace {

        constexpr size_t RawScreenRgbaBufferSize = 1280 * 720 * 4;

    }

    std::string GetLanguageString(const std::string &name) {
        return cfg::GetLanguageString(g_MainLanguage, g_DefaultLanguage, name);
    }

    void OnMessage(const smi::MenuMessageContext msg_ctx) {
        g_MenuApplication->GetLayout<IMenuLayout>()->NotifyMessageContext(msg_ctx);
    }

    void MenuApplication::OnLoad() {
        u8 *screen_capture_buf = nullptr;
        if(this->IsSuspended()) {
            screen_capture_buf = new u8[RawScreenRgbaBufferSize]();
            bool flag;
            appletGetLastApplicationCaptureImageEx(screen_capture_buf, RawScreenRgbaBufferSize, &flag);
        }

        this->bgm_json = ul::util::JSON::object();
        ul::util::LoadJSONFromFile(this->bgm_json, cfg::GetAssetByTheme(g_Theme, "sound/BGM.json"));
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
        this->main_menu_lyt = MainMenuLayout::New(screen_capture_buf, suspended_final_alpha);
        this->theme_menu_lyt = ThemeMenuLayout::New();
        this->settings_menu_lyt = SettingsMenuLayout::New();
        this->languages_menu_lyt = LanguagesMenuLayout::New();

        switch(this->start_mode) {
            case smi::MenuStartMode::StartupScreen: {
                this->LoadStartupMenu();
                break;
            }
            default: {
                this->StartPlayBGM();
                this->LoadMainMenu();
                break;
            }
        }
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
        this->system_status.selected_user = user_id;

        UL_RC_ASSERT(ul::menu::smi::SetSelectedUser(user_id));
    }

}