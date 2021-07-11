#include <ui/ui_StartupLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::MenuApplication::Ref g_MenuApplication;
extern cfg::Theme g_Theme;
extern cfg::Config g_Config;

namespace ui {

    StartupLayout::StartupLayout() {
        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));
        this->loadmenu = false;

        auto textclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        auto menufocusclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        auto menubgclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->infoText = pu::ui::elm::TextBlock::New(35, 650, GetLanguageString("startup_welcome_info"));
        this->infoText->SetColor(textclr);
        g_MenuApplication->ApplyConfigForElement("startup_menu", "info_text", this->infoText);
        this->Add(this->infoText);

        this->usersMenu = pu::ui::elm::Menu::New(200, 60, 880, menubgclr, 100, 5);
        this->usersMenu->SetOnFocusColor(menufocusclr);
        g_MenuApplication->ApplyConfigForElement("startup_menu", "users_menu_item", this->usersMenu);
        this->Add(this->usersMenu);
    }

    void StartupLayout::OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) {
        if(this->loadmenu) {
            this->loadmenu = false;
            g_MenuApplication->StartPlayBGM();
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadMenu();
            g_MenuApplication->FadeIn();
        }
    }

    void StartupLayout::OnHomeButtonPress() {
        // ...
    }

    void StartupLayout::user_Click(AccountUid uid) {
        this->loadmenu = true;
        g_MenuApplication->SetSelectedUser(uid);
    }

    void StartupLayout::create_Click() {
        auto rc = pselShowUserCreator();
        if(R_SUCCEEDED(rc)) {
            g_MenuApplication->FadeOut();
            this->ReloadMenu();
            g_MenuApplication->FadeIn();
        }
    }

    void StartupLayout::ReloadMenu() {
        this->usersMenu->ClearItems();
        this->usersMenu->SetSelectedIndex(0);

        auto textclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        std::vector<AccountUid> users;
        auto rc = os::QuerySystemAccounts(users, true);
        if(R_SUCCEEDED(rc)) {
            for(auto &user: users) {
                std::string name;
                auto rc = os::GetAccountName(name, user);
                if(R_SUCCEEDED(rc)) {
                    auto path = os::GetIconCacheImagePath(user);
                    auto uitm = pu::ui::elm::MenuItem::New(name);
                    uitm->SetIcon(path);
                    uitm->AddOnClick(std::bind(&StartupLayout::user_Click, this, user));
                    uitm->SetColor(textclr);

                    this->usersMenu->AddItem(uitm);
                }
            }
        }

        auto citm = pu::ui::elm::MenuItem::New(GetLanguageString("startup_new_user"));
        citm->SetColor(textclr);
        citm->AddOnClick(std::bind(&StartupLayout::create_Click, this));
        this->usersMenu->AddItem(citm);
    }

}