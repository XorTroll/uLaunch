#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

extern ul::menu::ui::GlobalSettings g_GlobalSettings;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    namespace {

        constexpr size_t RawScreenRgbaBufferSize = 1280 * 720 * 4;

    }

    std::string GetLanguageString(const std::string &name) {
        return cfg::GetLanguageString(g_GlobalSettings.main_lang, g_GlobalSettings.default_lang, name);
    }

    void OnMessage(const smi::MenuMessageContext &msg_ctx) {
        g_MenuApplication->GetLayout<IMenuLayout>()->NotifyMessageContext(msg_ctx);
    }

    MenuBgmEntry &MenuApplication::GetCurrentMenuBgm() {
        switch(this->loaded_menu) {
            case MenuType::Main:
                return g_GlobalSettings.main_menu_bgm;
            case MenuType::Startup:
                return g_GlobalSettings.startup_menu_bgm;
            case MenuType::Themes:
                return g_GlobalSettings.themes_menu_bgm;
            case MenuType::Settings:
                return g_GlobalSettings.settings_menu_bgm;
            case MenuType::Lockscreen:
                return g_GlobalSettings.lockscreen_menu_bgm;
        }

        UL_ASSERT_FAIL("Invalid current menu?");
    }

    void MenuApplication::OnLoad() {
        this->launch_failed = false;
        this->pending_gc_mount_rc = ResultSuccess;
        this->needs_app_records_reload = false;
        this->needs_app_entries_reload = false;
        memset(this->chosen_hb, 0, sizeof(this->chosen_hb));
        this->verify_finished_app_id = 0;
        this->verify_rc = ResultSuccess;
        this->verify_detail_rc = ResultSuccess;

        // TODO: customize
        this->SetFadeAlphaIncrementStepCount(FastFadeAlphaIncrementSteps);

        u8 *screen_capture_buf = nullptr;
        if(g_GlobalSettings.IsSuspended()) {
            screen_capture_buf = new u8[RawScreenRgbaBufferSize]();
            bool flag;
            appletGetLastApplicationCaptureImageEx(screen_capture_buf, RawScreenRgbaBufferSize, &flag);
        }

        InitializeResources();

        // BGM

        bool global_bgm_loop;
        if(TryGetBgmValue("bgm_loop", global_bgm_loop)) {
            g_GlobalSettings.main_menu_bgm.bgm_loop = global_bgm_loop;
            g_GlobalSettings.startup_menu_bgm.bgm_loop = global_bgm_loop;
            g_GlobalSettings.themes_menu_bgm.bgm_loop = global_bgm_loop;
            g_GlobalSettings.settings_menu_bgm.bgm_loop = global_bgm_loop;
            g_GlobalSettings.lockscreen_menu_bgm.bgm_loop = global_bgm_loop;
        }

        u32 global_bgm_fade_in_ms;
        if(TryGetBgmValue("bgm_fade_in_ms", global_bgm_fade_in_ms)) {
            g_GlobalSettings.main_menu_bgm.bgm_fade_in_ms = global_bgm_fade_in_ms;
            g_GlobalSettings.startup_menu_bgm.bgm_fade_in_ms = global_bgm_fade_in_ms;
            g_GlobalSettings.themes_menu_bgm.bgm_fade_in_ms = global_bgm_fade_in_ms;
            g_GlobalSettings.settings_menu_bgm.bgm_fade_in_ms = global_bgm_fade_in_ms;
            g_GlobalSettings.lockscreen_menu_bgm.bgm_fade_in_ms = global_bgm_fade_in_ms;
        }

        u32 global_bgm_fade_out_ms;
        if(TryGetBgmValue("bgm_fade_out_ms", global_bgm_fade_out_ms)) {
            g_GlobalSettings.main_menu_bgm.bgm_fade_out_ms = global_bgm_fade_out_ms;
            g_GlobalSettings.startup_menu_bgm.bgm_fade_out_ms = global_bgm_fade_out_ms;
            g_GlobalSettings.themes_menu_bgm.bgm_fade_out_ms = global_bgm_fade_out_ms;
            g_GlobalSettings.settings_menu_bgm.bgm_fade_out_ms = global_bgm_fade_out_ms;
            g_GlobalSettings.lockscreen_menu_bgm.bgm_fade_out_ms = global_bgm_fade_out_ms;
        }
        
        TryParseBgmEntry("main_menu", "Main", g_GlobalSettings.main_menu_bgm);
        TryParseBgmEntry("startup_menu", "Startup", g_GlobalSettings.startup_menu_bgm);
        TryParseBgmEntry("themes_menu", "Themes", g_GlobalSettings.themes_menu_bgm);
        TryParseBgmEntry("settings_menu", "Settings", g_GlobalSettings.settings_menu_bgm);
        TryParseBgmEntry("lockscreen_menu", "Lockscreen", g_GlobalSettings.lockscreen_menu_bgm);

        // UI

        const auto toast_text_clr = GetRequiredUiValue<pu::ui::Color>("toast_text_color");
        const auto toast_base_clr = GetRequiredUiValue<pu::ui::Color>("toast_base_color");

        auto notif_toast_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        notif_toast_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        notif_toast_text->SetColor(toast_text_clr);
        this->notif_toast = pu::ui::extras::Toast::New(notif_toast_text, toast_base_clr);

        this->text_clr = GetRequiredUiValue<pu::ui::Color>("text_color");

        this->menu_focus_clr = GetRequiredUiValue<pu::ui::Color>("menu_focus_color");
        this->menu_bg_clr = GetRequiredUiValue<pu::ui::Color>("menu_bg_color");

        this->dialog_title_clr = GetRequiredUiValue<pu::ui::Color>("dialog_title_color");
        this->dialog_cnt_clr = GetRequiredUiValue<pu::ui::Color>("dialog_cnt_color");
        this->dialog_opt_clr = GetRequiredUiValue<pu::ui::Color>("dialog_opt_color");
        this->dialog_clr = GetRequiredUiValue<pu::ui::Color>("dialog_color");
        this->dialog_over_clr = GetRequiredUiValue<pu::ui::Color>("dialog_over_color");

        const auto suspended_app_final_alpha = GetRequiredUiValue<u32>("suspended_app_final_alpha");

        switch(this->start_mode) {
            case smi::MenuStartMode::Start: {
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
        this->lockscreen_menu_lyt = LockscreenMenuLayout::New();

        this->loaded_menu = MenuType::Main;
        switch(this->start_mode) {
            case smi::MenuStartMode::Start: {
                bool lockscreen_enabled;
                UL_ASSERT_TRUE(g_GlobalSettings.config.GetEntry(cfg::ConfigEntryId::LockscreenEnabled, lockscreen_enabled));
                if(lockscreen_enabled) {
                    this->LoadMenuByType(MenuType::Lockscreen, false);
                }
                else {
                    this->LoadMenuByType(MenuType::Startup, false);
                }
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
                this->main_menu_lyt->NotifyLoad();
                this->LoadLayout(this->main_menu_lyt);
                break;
            }
            case MenuType::Settings: {
                this->settings_menu_lyt->Reload(false);
                this->LoadLayout(this->settings_menu_lyt);
                break;
            }
            case MenuType::Themes: {
                this->themes_menu_lyt->Reload();
                this->LoadLayout(this->themes_menu_lyt);
                break;
            }
            case MenuType::Lockscreen: {
                this->LoadLayout(this->lockscreen_menu_lyt);
                break;
            }
        }

        this->loaded_menu = type;

        this->StartPlayBgm();

        if(fade) {
            this->FadeIn();
        }
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
