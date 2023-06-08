#include <ul/menu/ui/ui_ThemeMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::menu::ui::TransitionGuard g_TransitionGuard;
extern ul::cfg::Theme g_Theme;
extern ul::cfg::Config g_Config;

namespace ul::menu::ui {

    ThemeMenuLayout::ThemeMenuLayout() {
        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));

        this->cur_theme_banner = pu::ui::elm::Image::New(0, 585, cfg::GetAssetByTheme(g_Theme, "ui/BannerTheme.png"));
        g_MenuApplication->ApplyConfigForElement("themes_menu", "banner_image", this->cur_theme_banner);
        this->Add(this->cur_theme_banner);

        this->themes_menu = pu::ui::elm::Menu::New(200, 60, 880, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), 100, 5);
        g_MenuApplication->ApplyConfigForElement("themes_menu", "themes_menu_item", this->themes_menu);
        this->Add(this->themes_menu);

        this->cur_theme_text = pu::ui::elm::TextBlock::New(20, 540, GetLanguageString("theme_current") + ":");
        this->cur_theme_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
        this->cur_theme_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_text", this->cur_theme_text);
        this->Add(this->cur_theme_text);

        this->cur_theme_name_text = pu::ui::elm::TextBlock::New(40, 610, "");
        this->cur_theme_name_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
        this->cur_theme_name_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_name_text", this->cur_theme_name_text);
        this->Add(this->cur_theme_name_text);

        this->cur_theme_author_text = pu::ui::elm::TextBlock::New(45, 650, "");
        this->cur_theme_author_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->cur_theme_author_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_author_text", this->cur_theme_author_text);
        this->Add(this->cur_theme_author_text);

        this->cur_theme_version_text = pu::ui::elm::TextBlock::New(45, 675, "");
        this->cur_theme_version_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
        this->cur_theme_version_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_version_text", this->cur_theme_version_text);
        this->Add(this->cur_theme_version_text);

        this->cur_theme_icon = pu::ui::elm::Image::New(1000, 605, "");
        this->cur_theme_icon->SetWidth(100);
        this->cur_theme_icon->SetHeight(100);
        g_MenuApplication->ApplyConfigForElement("themes_menu", "current_theme_icon", this->cur_theme_icon);
        this->Add(this->cur_theme_icon);
    }

    void ThemeMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(keys_down & HidNpadButton_B) {
            g_TransitionGuard.Run([]() {
                g_MenuApplication->FadeOut();
                g_MenuApplication->LoadMenu();
                g_MenuApplication->FadeIn();
            });
        }
    }

    bool ThemeMenuLayout::OnHomeButtonPress() {
        return g_TransitionGuard.Run([]() {
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadMenu();
            g_MenuApplication->FadeIn();
        });
    }

    void ThemeMenuLayout::Reload() {
        if(g_Theme.IsDefault()) {
            this->cur_theme_text->SetText(GetLanguageString("theme_no_custom"));
            this->cur_theme_name_text->SetVisible(false);
            this->cur_theme_author_text->SetVisible(false);
            this->cur_theme_version_text->SetVisible(false);
            this->cur_theme_banner->SetVisible(false);
            this->cur_theme_icon->SetVisible(false);
        }
        else {
            this->cur_theme_text->SetText(GetLanguageString("theme_current") + ":");
            this->cur_theme_name_text->SetVisible(true);
            this->cur_theme_name_text->SetText(g_Theme.manifest.name);
            this->cur_theme_author_text->SetVisible(true);
            this->cur_theme_author_text->SetText(g_Theme.manifest.author);
            this->cur_theme_version_text->SetVisible(true);
            this->cur_theme_version_text->SetText("v" + g_Theme.manifest.release);
            this->cur_theme_banner->SetVisible(true);
            this->cur_theme_icon->SetVisible(true);
            this->cur_theme_icon->SetImage(g_Theme.path + "/theme/Icon.png");
            this->cur_theme_icon->SetWidth(100);
            this->cur_theme_icon->SetHeight(100);
        }
        this->themes_menu->ClearItems();
        this->loaded_themes.clear();
        
        this->loaded_themes = cfg::LoadThemes();

        auto theme_reset_item = pu::ui::elm::MenuItem::New(GetLanguageString("theme_reset"));
        theme_reset_item->AddOnKey(std::bind(&ThemeMenuLayout::theme_DefaultKey, this));
        theme_reset_item->SetColor(g_MenuApplication->GetTextColor());
        theme_reset_item->SetIcon("romfs:/Logo.png");
        this->themes_menu->AddItem(theme_reset_item);
        
        for(const auto &theme: this->loaded_themes) {
            auto theme_item = pu::ui::elm::MenuItem::New(theme.manifest.name + " (v" + theme.manifest.release + ", " + GetLanguageString("theme_by") + " " + theme.manifest.author + ")");
            theme_item->AddOnKey(std::bind(&ThemeMenuLayout::theme_DefaultKey, this));
            theme_item->SetColor(g_MenuApplication->GetTextColor());
            theme_item->SetIcon(theme.path + "/theme/Icon.png");
            this->themes_menu->AddItem(theme_item);
        }

        this->themes_menu->SetSelectedIndex(0);
    }

    void ThemeMenuLayout::theme_DefaultKey() {
        const auto idx = this->themes_menu->GetSelectedIndex();
        if(idx == 0) {
            if(g_Theme.IsDefault()) {
                g_MenuApplication->ShowNotification(GetLanguageString("theme_no_custom"));
            }
            else {
                const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("theme_reset"), GetLanguageString("theme_reset_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                if(option == 0) {
                    UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::ActiveThemeName, std::string()));
                    cfg::SaveConfig(g_Config);

                    g_MenuApplication->StopPlayBGM();
                    g_MenuApplication->CloseWithFadeOut();
                    g_MenuApplication->ShowNotification(GetLanguageString("theme_changed"));

                    UL_RC_ASSERT(ul::menu::smi::RestartMenu());
                }
            }
        }
        else {
            const auto selected_theme = this->loaded_themes.at(idx - 1);
            const auto theme_icon_path = selected_theme.path + "/theme/Icon.png";
            if(selected_theme.base_name == g_Theme.base_name) {
                g_MenuApplication->ShowNotification(GetLanguageString("theme_active_this"));
            }
            else {
                const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("theme_set"), GetLanguageString("theme_set_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true, theme_icon_path);
                if(option == 0) {
                    UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::ActiveThemeName, selected_theme.base_name));
                    cfg::SaveConfig(g_Config);

                    g_MenuApplication->StopPlayBGM();
                    g_MenuApplication->CloseWithFadeOut();
                    g_MenuApplication->ShowNotification(GetLanguageString("theme_changed"));

                    UL_RC_ASSERT(ul::menu::smi::RestartMenu());
                }
            }
        }
    }

}