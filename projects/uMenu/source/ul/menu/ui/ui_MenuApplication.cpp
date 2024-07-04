#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

extern ul::cfg::Config g_Config;

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
        this->pending_gc_mount_rc = ResultSuccess;
        LoadCommonTextures();

        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, this->takeover_app_id));

        u8 *screen_capture_buf = nullptr;
        if(this->IsSuspended()) {
            screen_capture_buf = new u8[RawScreenRgbaBufferSize]();
            bool flag;
            appletGetLastApplicationCaptureImageEx(screen_capture_buf, RawScreenRgbaBufferSize, &flag);
        }

        this->bgm_json = ul::util::JSON::object();
        const auto rc = ul::util::LoadJSONFromFile(this->bgm_json, TryGetActiveThemeResource("sound/BGM.json"));
        if(R_SUCCEEDED(rc)) {
            #define _LOAD_MENU_BGM(menu, bgm_name) { \
                if(this->bgm_json.count(#menu)) { \
                    const auto menu_json = this->bgm_json[#menu]; \
                    this->menu##_bgm.bgm_loop = menu_json.value("bgm_loop", DefaultBgmLoop); \
                    this->menu##_bgm.bgm_fade_in_ms = menu_json.value("bgm_fade_in_ms", DefaultBgmFadeInMs); \
                    this->menu##_bgm.bgm_fade_out_ms = menu_json.value("bgm_fade_out_ms", DefaultBgmFadeOutMs); \
                    this->menu##_bgm.bgm = nullptr; \
                    this->menu##_bgm.bgm = pu::audio::OpenMusic(TryGetActiveThemeResource("sound/" bgm_name "/Bgm.mp3")); \
                } \
            }
            _LOAD_MENU_BGM(main_menu, "Main")
            _LOAD_MENU_BGM(startup_menu, "Startup")
            _LOAD_MENU_BGM(themes_menu, "Themes")
            _LOAD_MENU_BGM(settings_menu, "Settings")

            if(this->bgm_json.count("bgm_loop")) {
                const auto global_loop = this->bgm_json.value("bgm_loop", DefaultBgmLoop);
                this->main_menu_bgm.bgm_loop = global_loop;
                this->startup_menu_bgm.bgm_loop = global_loop;
                this->themes_menu_bgm.bgm_loop = global_loop;
                this->settings_menu_bgm.bgm_loop = global_loop;
            }
            if(this->bgm_json.count("bgm_fade_in_ms")) {
                const auto global_fade_in_ms = this->bgm_json.value("bgm_fade_in_ms", DefaultBgmFadeInMs);
                this->main_menu_bgm.bgm_fade_in_ms = global_fade_in_ms;
                this->startup_menu_bgm.bgm_fade_in_ms = global_fade_in_ms;
                this->themes_menu_bgm.bgm_fade_in_ms = global_fade_in_ms;
                this->settings_menu_bgm.bgm_fade_in_ms = global_fade_in_ms;
            }
            if(this->bgm_json.count("bgm_fade_out_ms")) {
                const auto global_fade_out_ms = this->bgm_json.value("bgm_fade_out_ms", DefaultBgmFadeOutMs);
                this->main_menu_bgm.bgm_fade_out_ms = global_fade_out_ms;
                this->startup_menu_bgm.bgm_fade_out_ms = global_fade_out_ms;
                this->themes_menu_bgm.bgm_fade_out_ms = global_fade_out_ms;
                this->settings_menu_bgm.bgm_fade_out_ms = global_fade_out_ms;
            }
        }
        else {
            UL_LOG_WARN("Unable to load active theme sound settings (%s), theme might have no sound features", util::FormatResultDisplay(rc).c_str());
        }

        // These UI values are required, we will assert otherwise (thus the error will be visible on our logs)

        #define _GET_UI_VALUE(name, type, val) { \
            UL_ASSERT_TRUE(this->ui_json.count(name)); \
            val = this->ui_json[name].get<type>(); \
        }
        #define _GET_UI_COLOR(name, val) { \
            std::string tmp_clr; \
            _GET_UI_VALUE(name, std::string, tmp_clr); \
            val = pu::ui::Color::FromHex(tmp_clr); \
        }

        pu::ui::Color toast_text_clr;
        _GET_UI_COLOR("toast_text_color", toast_text_clr);
        pu::ui::Color toast_base_clr;
        _GET_UI_COLOR("toast_base_color", toast_base_clr);

        auto notif_toast_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        notif_toast_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        notif_toast_text->SetColor(toast_text_clr);
        this->notif_toast = pu::ui::extras::Toast::New(notif_toast_text, toast_base_clr);

        _GET_UI_COLOR("text_color", this->text_clr);

        _GET_UI_COLOR("menu_focus_color", this->menu_focus_clr);
        _GET_UI_COLOR("menu_bg_color", this->menu_bg_clr);

        _GET_UI_COLOR("dialog_title_color", this->dialog_title_clr);
        _GET_UI_COLOR("dialog_cnt_color", this->dialog_cnt_clr);
        _GET_UI_COLOR("dialog_opt_color", this->dialog_opt_clr);
        _GET_UI_COLOR("dialog_color", this->dialog_clr);
        _GET_UI_COLOR("dialog_over_color", this->dialog_over_clr);

        this->launch_failed = false;
        memset(this->chosen_hb, 0, sizeof(this->chosen_hb));

        u32 suspended_app_final_alpha;
        _GET_UI_VALUE("suspended_app_final_alpha", u32, suspended_app_final_alpha);

        switch(this->start_mode) {
            case smi::MenuStartMode::StartupMenu: {
                break;
            }
            default: {
                LoadSelectedUserIconTexture();
                break;
            }
        }

        // TODO (low priority): do not create all layouts, only the loaded one?
        this->startup_menu_lyt = StartupMenuLayout::New();
        this->main_menu_lyt = MainMenuLayout::New(screen_capture_buf, static_cast<u8>(suspended_app_final_alpha));
        this->themes_menu_lyt = ThemesMenuLayout::New();
        this->settings_menu_lyt = SettingsMenuLayout::New();

        switch(this->start_mode) {
            case smi::MenuStartMode::StartupMenu: {
                this->LoadMenuByType(MenuType::Startup, false);
                break;
            }
            case smi::MenuStartMode::SettingsMenu: {
                this->LoadMenuByType(MenuType::Settings, false);
                break;
            }
            default: {
                this->LoadMenuByType(MenuType::Main, false);
                break;
            }
        }
        this->StartPlayBgm();
    }

    void MenuApplication::SetBackgroundFade() {
        if(!this->HasFadeBackgroundImage()) {
            this->SetFadeBackgroundImage(GetBackgroundTexture());
        }
    }

    void MenuApplication::LoadMenuByType(const MenuType type, const bool fade, MenuFadeCallback fade_cb) {
        this->StopPlayBgm();

        if(fade) {
            this->SetBackgroundFade();
            this->FadeOut();

            if(fade_cb) {
                fade_cb();
            }
        }
        
        switch(type) {
            case MenuType::Startup: {
                this->startup_menu_lyt->ReloadMenu();
                this->LoadLayout(this->startup_menu_lyt);
                break;
            }
            case MenuType::Main: {
                this->main_menu_lyt->NotifyLoad(this->system_status.selected_user);
                this->LoadLayout(this->main_menu_lyt);
                break;
            }
            case MenuType::Settings: {
                this->settings_menu_lyt->Reload(true);
                this->LoadLayout(this->settings_menu_lyt);
                break;
            }
            case MenuType::Themes: {
                this->themes_menu_lyt->Reload();
                this->LoadLayout(this->themes_menu_lyt);
                break;
            }
        }

        this->loaded_menu = type;

        this->StartPlayBgm();

        if(fade) {
            this->FadeIn();
        }
    }

    void MenuApplication::SetTakeoverApplicationId(const u64 app_id) {
        this->takeover_app_id = app_id;

        // No need to save config, uSystem deals with that
        UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, this->takeover_app_id));
        SaveConfig();
    }

    void MenuApplication::ShowNotification(const std::string &text, const u64 timeout) {
        this->EndOverlay();
        this->notif_toast->SetText(text);
        this->StartOverlayWithTimeout(this->notif_toast, timeout);
    }

    void MenuApplication::StartPlayBgm() {
        const auto &bgm = this->GetCurrentMenuBgm();
        if(bgm.bgm != nullptr) {
            const int loops = bgm.bgm_loop ? -1 : 1;
            if(bgm.bgm_fade_in_ms > 0) {
                pu::audio::PlayMusicWithFadeIn(bgm.bgm, loops, bgm.bgm_fade_in_ms);
            }
            else {
                pu::audio::PlayMusic(bgm.bgm, loops);
            }
        }
    }

    void MenuApplication::StopPlayBgm() {
        const auto &bgm = this->GetCurrentMenuBgm();
        if(bgm.bgm_fade_out_ms > 0) {
            pu::audio::FadeOutMusic(bgm.bgm_fade_out_ms);
        }
        else {
            pu::audio::StopMusic();
        }
    }
    
    void MenuApplication::SetSelectedUser(const AccountUid user_id) {
        this->system_status.selected_user = user_id;
        UL_RC_ASSERT(ul::menu::smi::SetSelectedUser(user_id));
        LoadSelectedUserIconTexture();
    }

    int MenuApplication::DisplayDialog(const std::string &title, const std::string &content, const std::vector<std::string> &opts, const bool use_last_opt_as_cancel, pu::sdl2::TextureHandle::Ref icon) {
        return this->CreateShowDialog(title, content, opts, use_last_opt_as_cancel, icon, [&](pu::ui::Dialog::Ref &dialog) {
            dialog->SetTitleColor(this->dialog_title_clr);
            dialog->SetContentColor(this->dialog_title_clr);
            dialog->SetOptionColor(this->dialog_opt_clr);
            dialog->SetDialogColor(this->dialog_clr);
            dialog->SetOverColor(this->dialog_over_clr);
        });
    }

}
