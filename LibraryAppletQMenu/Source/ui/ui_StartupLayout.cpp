#include <ui/ui_StartupLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_QMenuApplication.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::QMenuApplication::Ref qapp;
extern cfg::Theme theme;
extern cfg::Config config;

namespace ui
{
    StartupLayout::StartupLayout()
    {
        this->SetBackgroundImage(cfg::GetAssetByTheme(theme, "ui/Background.png"));
        this->loadmenu = false;

        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        pu::ui::Color menufocusclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        pu::ui::Color menubgclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->infoText = pu::ui::elm::TextBlock::New(35, 635, cfg::GetLanguageString(config.main_lang, config.default_lang, "startup_welcome_info") + "\n" + cfg::GetLanguageString(config.main_lang, config.default_lang, "startup_control_info"));
        this->infoText->SetColor(textclr);
        qapp->ApplyConfigForElement("startup_menu", "info_text", this->infoText);
        this->Add(this->infoText);

        this->usersMenu = pu::ui::elm::Menu::New(200, 60, 880, menubgclr, 100, 5);
        this->usersMenu->SetOnFocusColor(menufocusclr);
        qapp->ApplyConfigForElement("startup_menu", "users_menu_item", this->usersMenu);
        this->Add(this->usersMenu);

        this->SetOnInput(std::bind(&StartupLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void StartupLayout::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos)
    {
        if(this->loadmenu)
        {
            this->loadmenu = false;
            qapp->StartPlayBGM();
            qapp->FadeOut();
            qapp->LoadMenu();
            qapp->FadeIn();
            qapp->ShowNotification(cfg::GetLanguageString(config.main_lang, config.default_lang, "menu_quick_info"), 3000); // Show for 3s
        }
    }

    void StartupLayout::user_Click(u128 uid, bool has_password)
    {
        bool login_ok = false;
        if(has_password)
        {
            SwkbdConfig swkbd;
            swkbdCreate(&swkbd, 0);
            swkbdConfigMakePresetPassword(&swkbd);
            swkbdConfigSetStringLenMax(&swkbd, 15);
            swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(config.main_lang, config.default_lang, "swkbd_user_pass_guide").c_str());
            char inpass[0x10] = {0};
            auto rc = swkbdShow(&swkbd, inpass, 0x10);
            swkbdClose(&swkbd);
            if(R_SUCCEEDED(rc))
            {
                auto [rc2, pass] = db::PackPassword(uid, inpass);
                rc = rc2;
                if(R_SUCCEEDED(rc))
                {
                    am::QMenuCommandWriter writer(am::QDaemonMessage::TryLogUser);
                    writer.Write<db::PassBlock>(pass);
                    writer.FinishWrite();

                    am::QMenuCommandResultReader reader;
                    rc = reader.GetReadResult();
                    reader.FinishRead();

                    if(R_FAILED(rc)) qapp->ShowNotification(cfg::GetLanguageString(config.main_lang, config.default_lang, "startup_login_error"));
                    else login_ok = true;
                }
            }
        }
        else login_ok = true;
        this->loadmenu = login_ok;
        if(login_ok) qapp->SetSelectedUser(uid);
    }

    void StartupLayout::create_Click()
    {
        u8 in[0xa0] = {0};
        u8 out[0x18] = {0};
        *(u32*)in = 1; // PselUiMode_UserCreation
        LibAppletArgs args;
        libappletArgsCreate(&args, 0);
        auto rc = libappletLaunch(AppletId_playerSelect, &args, in, 0xa0, out, 0x18, NULL);
        if(R_SUCCEEDED(rc))
        {
            rc = *(u32*)out;
            if(R_SUCCEEDED(rc))
            {
                qapp->FadeOut();
                this->ReloadMenu();
                qapp->FadeIn();
            }
        }
    }

    void StartupLayout::ReloadMenu()
    {
        this->usersMenu->ClearItems();
        this->usersMenu->SetSelectedIndex(0);

        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        
        auto [rc, users] = os::QuerySystemAccounts(true);
        if(R_SUCCEEDED(rc))
        {
            for(auto user: users)
            {
                auto [rc, name] = os::GetAccountName(user);
                if(R_FAILED(rc)) continue;

                am::QMenuCommandWriter writer(am::QDaemonMessage::UserHasPassword);
                writer.Write<u128>(user);
                writer.FinishWrite();

                am::QMenuCommandResultReader res;
                res.FinishRead();

                bool has_pass = R_SUCCEEDED(res.GetReadResult());
                if(has_pass) name += " " + cfg::GetLanguageString(config.main_lang, config.default_lang, "startup_password");

                auto path = os::GetIconCacheImagePath(user);
                auto uitm = pu::ui::elm::MenuItem::New(name);
                uitm->SetIcon(path);
                uitm->AddOnClick(std::bind(&StartupLayout::user_Click, this, user, has_pass));
                uitm->SetColor(textclr);

                this->usersMenu->AddItem(uitm);
            }
        }

        auto citm = pu::ui::elm::MenuItem::New(cfg::GetLanguageString(config.main_lang, config.default_lang, "startup_new_user"));
        citm->SetColor(textclr);
        citm->AddOnClick(std::bind(&StartupLayout::create_Click, this));
        this->usersMenu->AddItem(citm);
    }
}