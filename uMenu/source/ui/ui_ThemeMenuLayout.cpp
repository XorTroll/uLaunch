#include <ui/ui_ThemeMenuLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::MenuApplication::Ref g_MenuApplication;
extern cfg::Theme g_Theme;
extern cfg::Config g_Config;

namespace ui {

    ThemeMenuLayout::ThemeMenuLayout() {
        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));

        auto textclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        auto menufocusclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        auto menubgclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->curThemeBanner = pu::ui::elm::Image::New(0, 585, cfg::GetAssetByTheme(g_Theme, "ui/BannerTheme.png"));
        g_MenuApplication->ApplyConfigForElement("themes_menu", "banner_image", this->curThemeBanner);
        this->Add(this->curThemeBanner);

        this->themesMenu = pu::ui::elm::Menu::New(200, 60, 880, menubgclr, 100, 5);
        this->themesMenu->SetOnFocusColor(menufocusclr);
        g_MenuApplication->ApplyConfigForElement("themes_menu", "themes_menu_item", this->themesMenu);
        this->Add(this->themesMenu);

        this->curThemeText = pu::ui::elm::TextBlock::New(20, 540, GetLanguageString("theme_current") + ":");
        this->curThemeText->SetFont("DefaultFont@30");
        this->curThemeText->SetColor(textclr);
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_text", this->curThemeText);
        this->Add(this->curThemeText);
        
        this->curThemeName = pu::ui::elm::TextBlock::New(40, 610, "");
        this->curThemeName->SetFont("DefaultFont@30");
        this->curThemeName->SetColor(textclr);
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_name_text", this->curThemeName);
        this->Add(this->curThemeName);
        this->curThemeAuthor = pu::ui::elm::TextBlock::New(45, 650, "");
        this->curThemeAuthor->SetFont("DefaultFont@20");
        this->curThemeAuthor->SetColor(textclr);
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_author_text", this->curThemeAuthor);
        this->Add(this->curThemeAuthor);
        this->curThemeVersion = pu::ui::elm::TextBlock::New(45, 675, "");
        this->curThemeVersion->SetFont("DefaultFont@30");
        this->curThemeVersion->SetColor(textclr);
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_version_text", this->curThemeVersion);
        this->Add(this->curThemeVersion);
        this->curThemeIcon = pu::ui::elm::Image::New(1000, 605, "");
        this->curThemeIcon->SetWidth(100);
        this->curThemeIcon->SetHeight(100);
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_icon", this->curThemeIcon);
        this->Add(this->curThemeIcon);
    }

    void ThemeMenuLayout::OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) {
        if(down & HidNpadButton_B) {
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadMenu();
            g_MenuApplication->FadeIn();
        }
    }

    void ThemeMenuLayout::OnHomeButtonPress() {
        g_MenuApplication->FadeOut();
        g_MenuApplication->LoadMenu();
        g_MenuApplication->FadeIn();
    }

    void ThemeMenuLayout::Reload() {
        auto textclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        if(cfg::ThemeIsDefault(g_Theme)) {
            this->curThemeText->SetText(GetLanguageString("theme_no_custom"));
            this->curThemeName->SetVisible(false);
            this->curThemeAuthor->SetVisible(false);
            this->curThemeVersion->SetVisible(false);
            this->curThemeBanner->SetVisible(false);
            this->curThemeIcon->SetVisible(false);
        }
        else {
            this->curThemeText->SetText(GetLanguageString("theme_current") + ":");
            this->curThemeName->SetVisible(true);
            this->curThemeName->SetText(g_Theme.manifest.name);
            this->curThemeAuthor->SetVisible(true);
            this->curThemeAuthor->SetText(g_Theme.manifest.author);
            this->curThemeVersion->SetVisible(true);
            this->curThemeVersion->SetText("v" + g_Theme.manifest.release);
            this->curThemeBanner->SetVisible(true);
            this->curThemeIcon->SetVisible(true);
            this->curThemeIcon->SetImage(g_Theme.path + "/theme/Icon.png");
            this->curThemeIcon->SetWidth(100);
            this->curThemeIcon->SetHeight(100);
        }
        this->themesMenu->ClearItems();
        this->themesMenu->SetSelectedIndex(0);

        this->loadedThemes.clear();
        this->loadedThemes = cfg::LoadThemes();

        auto ditm = pu::ui::elm::MenuItem::New(GetLanguageString("theme_reset"));
        ditm->AddOnClick(std::bind(&ThemeMenuLayout::theme_Click, this));
        ditm->SetColor(textclr);
        ditm->SetIcon("romfs:/Logo.png");
        this->themesMenu->AddItem(ditm);
        
        for(auto &ltheme: this->loadedThemes) {
            auto itm = pu::ui::elm::MenuItem::New(ltheme.manifest.name + " (v" + ltheme.manifest.release + ", " + GetLanguageString("theme_by") + " " + ltheme.manifest.author + ")");
            itm->AddOnClick(std::bind(&ThemeMenuLayout::theme_Click, this));
            itm->SetColor(textclr);
            auto iconpath = ltheme.path + "/theme/Icon.png";
            itm->SetIcon(iconpath);
            this->themesMenu->AddItem(itm);
        }
    }

    void ThemeMenuLayout::theme_Click() {
        auto idx = this->themesMenu->GetSelectedIndex();
        if(idx == 0) {
            if(cfg::ThemeIsDefault(g_Theme)) {
                g_MenuApplication->ShowNotification(GetLanguageString("theme_no_custom"));
            }
            else {
                auto sopt = g_MenuApplication->CreateShowDialog(GetLanguageString("theme_reset"), GetLanguageString("theme_reset_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                if(sopt == 0) {
                    UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::ActiveThemeName, std::string()));
                    cfg::SaveConfig(g_Config);

                    g_MenuApplication->StopPlayBGM();
                    g_MenuApplication->CloseWithFadeOut();
                    g_MenuApplication->ShowNotification(GetLanguageString("theme_changed"));

                    UL_ASSERT(dmi::menu::SendCommand(dmi::DaemonMessage::RestartMenu, [&](dmi::menu::MenuScopedStorageWriter &writer) {
                        // ...
                        return ResultSuccess;
                    },
                    [&](dmi::menu::MenuScopedStorageReader &reader) {
                        // ...
                        return ResultSuccess;
                    }));
                }
            }
        }
        else {
            idx--;
            auto seltheme = this->loadedThemes[idx];
            auto iconpath = seltheme.path + "/theme/Icon.png";
            if(seltheme.base_name == g_Theme.base_name) {
                g_MenuApplication->ShowNotification(GetLanguageString("theme_active_this"));
            }
            else {
                auto sopt = g_MenuApplication->CreateShowDialog(GetLanguageString("theme_set"), GetLanguageString("theme_set_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true, iconpath);
                if(sopt == 0) {
                    UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::ActiveThemeName, seltheme.base_name));
                    cfg::SaveConfig(g_Config);

                    g_MenuApplication->StopPlayBGM();
                    g_MenuApplication->CloseWithFadeOut();
                    g_MenuApplication->ShowNotification(GetLanguageString("theme_changed"));

                    UL_ASSERT(dmi::menu::SendCommand(dmi::DaemonMessage::RestartMenu, [&](dmi::menu::MenuScopedStorageWriter &writer) {
                        // ...
                        return ResultSuccess;
                    },
                    [&](dmi::menu::MenuScopedStorageReader &reader) {
                        // ...
                        return ResultSuccess;
                    }));
                }
            }
        }
    }

}