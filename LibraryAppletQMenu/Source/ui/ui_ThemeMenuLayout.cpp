#include <ui/ui_ThemeMenuLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_QMenuApplication.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::QMenuApplication::Ref qapp;
extern cfg::ProcessedTheme theme;
extern cfg::Config config;

namespace ui
{
    ThemeMenuLayout::ThemeMenuLayout()
    {
        this->SetBackgroundImage(cfg::ProcessedThemeResource(theme, "ui/Background.png"));

        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        pu::ui::Color menufocusclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        pu::ui::Color menubgclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->noThemesText = pu::ui::elm::TextBlock::New(0, 0, "You don't seem to have any themes. Go download some!");
        this->noThemesText->SetColor(textclr);
        this->noThemesText->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->noThemesText->SetVerticalAlign(pu::ui::elm::VerticalAlign::Center);
        this->Add(this->noThemesText);

        this->themesMenu = pu::ui::elm::Menu::New(200, 160, 880, menubgclr, 100, 4);
        this->themesMenu->SetOnFocusColor(menufocusclr);
        this->Add(this->themesMenu);

        this->SetOnInput(std::bind(&ThemeMenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void ThemeMenuLayout::Reload()
    {
        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        this->themesMenu->ClearItems();
        this->loadedThemes.clear();
        this->loadedThemes = cfg::LoadThemes();
        this->noThemesText->SetVisible(this->loadedThemes.empty());
        this->themesMenu->SetVisible(!this->loadedThemes.empty());
        if(this->themesMenu->IsVisible())
        {
            for(auto &ltheme: this->loadedThemes)
            {
                auto itm = pu::ui::elm::MenuItem::New(ltheme.manifest.name + " (v" + ltheme.manifest.release + ", by " + ltheme.manifest.author + ")");
                itm->AddOnClick(std::bind(&ThemeMenuLayout::theme_Click, this));
                itm->SetColor(textclr);
                auto iconpath = ltheme.path + "/theme/Icon.png";
                itm->SetIcon(iconpath);
                this->themesMenu->AddItem(itm);
            }
        }
    }

    void ThemeMenuLayout::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos)
    {
        bool ret = false;
        auto [rc, msg] = am::QMenu_GetLatestQMenuMessage();
        switch(msg)
        {
            case am::QMenuMessage::HomeRequest:
            {
                ret = true;
                break;
            }
            default:
                break;
        }
        if(down & KEY_B) ret = true;
        if(ret)
        {
            qapp->FadeOut();
            qapp->LoadMenu();
            qapp->FadeIn();
        }
    }

    void ThemeMenuLayout::theme_Click()
    {
        auto seltheme = this->loadedThemes[this->themesMenu->GetSelectedIndex()];
        auto iconpath = seltheme.path + "/theme/Icon.png";
        auto sopt = qapp->CreateShowDialog("Set theme", "Would you like to set '" + seltheme.manifest.name + "' theme as uLaunch's theme?\nNote: changes require a reboot.", { "Yes", "Cancel" }, true, iconpath);
        if(sopt == 0)
        {
            config.theme_name = seltheme.base_name;
            cfg::SaveConfig(config);
            qapp->CreateShowDialog("Set theme", "uLaunch's theme was updated.", { "Ok" }, true);
        }
    }
}