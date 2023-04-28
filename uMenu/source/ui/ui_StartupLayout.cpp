#include <ui/ui_StartupLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <util/util_Misc.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::MenuApplication::Ref g_MenuApplication;
extern ui::TransitionGuard g_TransitionGuard;
extern cfg::Theme g_Theme;
extern cfg::Config g_Config;

namespace ui {

    void StartupLayout::user_DefaultKey(const AccountUid uid) {
        this->load_menu = true;
        g_MenuApplication->SetSelectedUser(uid);
    }

    void StartupLayout::create_DefaultKey() {
        if(R_SUCCEEDED(pselShowUserCreator())) {
            g_TransitionGuard.Run([&]() {
                g_MenuApplication->FadeOut();
                this->ReloadMenu();
                g_MenuApplication->FadeIn();
            });
        }
    }

    void StartupLayout::StartPlayBGM() {
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

    void StartupLayout::StopPlayBGM() {
        if(this->bgm_fade_out_ms > 0) {
            pu::audio::FadeOutMusic(this->bgm_fade_out_ms);
        }
        else {
            pu::audio::StopMusic();
        }
    }

    StartupLayout::StartupLayout() {

        this->bgm_json = JSON::object();
        util::LoadJSONFromFile(this->bgm_json, cfg::GetAssetByTheme(g_Theme, "sound/BGM.json"));
        this->bgm_loop = this->bgm_json.value("loop", true);
        this->bgm_fade_in_ms = this->bgm_json.value("fade_in_ms", 1500);
        this->bgm_fade_out_ms = this->bgm_json.value("fade_out_ms", 500);

        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));
        this->load_menu = false;

        this->bgm = pu::audio::OpenMusic(cfg::GetAssetByTheme(g_Theme, "sound/BGM_Login.mp3"));

        this->info_text = pu::ui::elm::TextBlock::New(35, 650, GetLanguageString("startup_welcome_info"));
        this->info_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("startup_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->users_menu = pu::ui::elm::Menu::New(200, 60, 880, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), 100, 5);
        g_MenuApplication->ApplyConfigForElement("startup_menu", "users_menu_item", this->users_menu);
        this->Add(this->users_menu);

        this->user_scroll_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/UserScroll.wav"));
        this->user_select_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/UserSelect.wav"));
    }

    void StartupLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->load_menu) {
            this->load_menu = false;
            g_MenuApplication->StartPlayBGM();
            g_TransitionGuard.Run([]() {
                g_MenuApplication->FadeOut();
                g_MenuApplication->LoadMenu();
                g_MenuApplication->FadeIn();
            });
        }else{
            if(keys_down & HidNpadButton_AnyUp || keys_down & HidNpadButton_AnyDown){
                pu::audio::PlaySfx(this->user_scroll_sfx); //When scrolling an user i want to play the sfx
            }else if(keys_down & HidNpadButton_A ){
                pu::audio::PlaySfx(this->user_select_sfx); //When selecting an user i want to play my sfx
                this->StopPlayBGM(); //If the user is selected i will stop my background music
            }
        }
    }

    bool StartupLayout::OnHomeButtonPress() {
        // ...
        return true;
    }

    void StartupLayout::ReloadMenu() {
        this->users_menu->ClearItems();

        std::vector<AccountUid> users;
        if(R_SUCCEEDED(os::QuerySystemAccounts(true, users))) {
            for(const auto &user: users) {
                std::string name;
                if(R_SUCCEEDED(os::GetAccountName(user, name))) {
                    const auto path = os::GetIconCacheImagePath(user);
                    auto user_item = pu::ui::elm::MenuItem::New(name);
                    user_item->SetIcon(path);
                    user_item->AddOnKey(std::bind(&StartupLayout::user_DefaultKey, this, user));
                    user_item->SetColor(g_MenuApplication->GetTextColor());
                    this->users_menu->AddItem(user_item);
                }
            }
        }

        auto create_user_item = pu::ui::elm::MenuItem::New(GetLanguageString("startup_new_user"));
        create_user_item->SetColor(g_MenuApplication->GetTextColor());
        create_user_item->AddOnKey(std::bind(&StartupLayout::create_DefaultKey, this));
        this->users_menu->AddItem(create_user_item);

        this->users_menu->SetSelectedIndex(0);
        this->StartPlayBGM(); //Start background music after menu reload
    }

}