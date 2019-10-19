#include <ui/ui_StartupLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_QMenuApplication.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::QMenuApplication::Ref qapp;
extern cfg::ProcessedTheme theme;

namespace ui
{
    StartupLayout::StartupLayout()
    {
        this->SetBackgroundImage(cfg::ProcessedThemeResource(theme, "ui/Background.png"));

        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        pu::ui::Color menufocusclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        pu::ui::Color menubgclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->infoText = pu::ui::elm::TextBlock::New(0, 100, "Welcome! Please select an account to use.");
        this->infoText->SetColor(textclr);
        this->infoText->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->Add(this->infoText);

        this->usersMenu = pu::ui::elm::Menu::New(400, 160, 480, menubgclr, 100, 4);
        this->usersMenu->SetOnFocusColor(menufocusclr);
        
        auto [rc, users] = os::QuerySystemAccounts(true);
        if(R_SUCCEEDED(rc))
        {
            for(auto user: users)
            {
                auto [rc, name] = os::GetAccountName(user);
                if(R_FAILED(rc)) continue;

                auto [passrc, pass] = db::AccessPassword(user);
                bool has_pass = R_SUCCEEDED(passrc);
                if(has_pass) name += " (password)";

                auto path = os::GetIconCacheImagePath(user);
                auto uitm = pu::ui::elm::MenuItem::New(name);
                uitm->SetIcon(path);
                uitm->AddOnClick(std::bind(&StartupLayout::user_Click, this));
                uitm->SetColor(textclr);

                this->userlist.push_back(user);
                this->passlist.push_back(has_pass);

                usersMenu->AddItem(uitm);
            }
        }
        else
        {
            this->usersMenu->SetVisible(false);
            this->infoText->SetText("Unable to obtain system accounts. Hold power to power off or reboot the console.");
        }

        this->SetOnInput([&](u64 down, u64 up, u64 held, pu::ui::Touch pos)
        {
            if(down & KEY_X)
            {
                fs::DeleteDirectory(Q_BASE_DB_DIR "/user");
                fs::CreateDirectory(Q_BASE_DB_DIR "/user");
                auto rc = db::RegisterUserPassword(this->userlist[0], "xor");
                qapp->CreateShowDialog("Re-register", "0x" + util::FormatApplicationId(rc), {"Ok"}, true);
            }
        });
        
        this->Add(this->usersMenu);
    }

    void StartupLayout::user_Click()
    {
        auto usridx = this->usersMenu->GetSelectedIndex();
        auto uid = this->userlist[usridx];
        auto haspass = this->passlist[usridx];
        bool login_ok = false;
        if(haspass)
        {
            SwkbdConfig swkbd;
            swkbdCreate(&swkbd, 0);
            swkbdConfigMakePresetPassword(&swkbd);
            swkbdConfigSetStringLenMax(&swkbd, 15);
            swkbdConfigSetGuideText(&swkbd, "User password");
            swkbdConfigSetHeaderText(&swkbd, "Input user password");
            char inpass[0x10] = {0};
            auto rc = swkbdShow(&swkbd, inpass, 0x10);
            if(R_SUCCEEDED(rc))
            {
                auto rc = db::TryLogUser(uid, std::string(inpass));
                if(R_FAILED(rc)) qapp->CreateShowDialog("Login", "Invalid password. Please try again.", {"Ok"}, true);
                else login_ok = true;
            }
            swkbdClose(&swkbd);
        }
        else login_ok = true;
        if(login_ok)
        {
            qapp->FadeOut();
            qapp->SetSelectedUser(uid);
            qapp->LoadMenu();
            qapp->FadeIn();
        }
    }
}