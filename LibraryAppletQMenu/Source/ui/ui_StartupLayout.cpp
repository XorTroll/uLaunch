#include <ui/ui_StartupLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_QMenuApplication.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::QMenuApplication::Ref qapp;

namespace ui
{
    StartupLayout::StartupLayout(pu::ui::Color bgcolor)
    {
        this->SetBackgroundColor(bgcolor);

        auto clr = bgcolor;
        if(clr.R < 255) clr.R += std::min(20, 255 - clr.R);
        if(clr.G < 255) clr.G += std::min(20, 255 - clr.G);
        if(clr.B < 255) clr.B += std::min(20, 255 - clr.B);

        this->infoText = pu::ui::elm::TextBlock::New(0, 100, "Welcome! Please select an account to use.");
        this->infoText->SetColor({ 235, 235, 235, 255 });
        this->infoText->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->Add(this->infoText);

        this->usersMenu = pu::ui::elm::Menu::New(400, 160, 480, bgcolor, 100, 4);
        this->usersMenu->SetOnFocusColor(clr);
        
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