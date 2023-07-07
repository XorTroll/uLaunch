#include <ul/menu/ui/ui_StartupLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/acc/acc_Accounts.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::menu::ui::TransitionGuard g_TransitionGuard;
extern ul::cfg::Theme g_Theme;
extern ul::cfg::Config g_Config;

namespace ul::menu::ui {

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

    StartupLayout::StartupLayout() {
        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));
        this->load_menu = false;

        this->info_text = pu::ui::elm::TextBlock::New(35, 650, GetLanguageString("startup_welcome_info"));
        this->info_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("startup_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->users_menu = pu::ui::elm::Menu::New(200, 60, 880, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), 100, 5);
        g_MenuApplication->ApplyConfigForElement("startup_menu", "users_menu_item", this->users_menu);
        this->Add(this->users_menu);
    }

    void StartupLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->load_menu) {
            this->load_menu = false;
            g_MenuApplication->StartPlayBGM();
            g_TransitionGuard.Run([]() {
                g_MenuApplication->FadeOut();
                g_MenuApplication->LoadMainMenu();
                g_MenuApplication->FadeIn();
            });
        }
    }

    bool StartupLayout::OnHomeButtonPress() {
        // ...
        return true;
    }

    void StartupLayout::ReloadMenu() {
        this->users_menu->ClearItems();

        std::vector<AccountUid> users;
        if(R_SUCCEEDED(acc::ListAccounts(users))) {
            for(const auto &user: users) {
                std::string name;
                if(R_SUCCEEDED(acc::GetAccountName(user, name))) {
                    auto user_item = pu::ui::elm::MenuItem::New(name);
                    user_item->SetIcon(acc::GetIconCacheImagePath(user));
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
    }

}