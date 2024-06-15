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

        auto notif_toast_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        notif_toast_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        notif_toast_text->SetColor(toast_text_clr);
        this->notif_toast = pu::ui::extras::Toast::New(notif_toast_text, toast_base_clr);

        this->bgm = pu::audio::OpenMusic(cfg::GetAssetByTheme(g_Theme, "sound/BGM.mp3"));

        this->text_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        this->menu_focus_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        this->menu_bg_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->dialog_title_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("dialog_title_color", "#0a0a0aff"));
        this->dialog_cnt_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("dialog_cnt_color", "#141414ff"));
        this->dialog_opt_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("dialog_opt_color", "#0a0a0aff"));
        this->dialog_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("dialog_color", "#e1e1e1ff"));
        this->dialog_over_clr = pu::ui::Color::FromHex(this->GetUIConfigValue<std::string>("dialog_over_color", "#b4b4c8ff"));

        this->launch_failed = false;
        memset(this->chosen_hb, 0, sizeof(this->chosen_hb));

        const u8 suspended_final_alpha = this->ui_json.value("suspended_final_alpha", 80);
        this->startup_lyt = StartupMenuLayout::New();
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

    int MenuApplication::DisplayDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool use_last_opt_as_cancel, const std::string &icon_path) {
        return this->CreateShowDialog(title, content, opts, use_last_opt_as_cancel, icon_path, [&](pu::ui::Dialog::Ref &dialog) {
            dialog->SetTitleColor(this->dialog_title_clr);
            dialog->SetContentColor(this->dialog_title_clr);
            dialog->SetOptionColor(this->dialog_opt_clr);
            dialog->SetDialogColor(this->dialog_clr);
            dialog->SetOverColor(this->dialog_over_clr);
        });
    }

}