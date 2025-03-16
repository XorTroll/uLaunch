#include <ul/menu/ui/ui_ThemesMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

extern ul::menu::ui::GlobalSettings g_GlobalSettings;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    ThemesMenuLayout::ThemesMenuLayout() : IMenuLayout() {
        this->info_text = pu::ui::elm::TextBlock::New(0, 0, GetLanguageString("theme_info_text"));
        this->info_text->SetColor(g_MenuApplication->GetTextColor());
        g_GlobalSettings.ApplyConfigForElement("themes_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->themes_menu = pu::ui::elm::Menu::New(0, 0, ThemesMenuWidth, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), ThemesMenuItemSize, ThemesMenuItemsToShow);
        g_GlobalSettings.ApplyConfigForElement("themes_menu", "themes_menu", this->themes_menu);
        this->Add(this->themes_menu);

        this->theme_change_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Themes/ThemeChange.wav"));
        this->back_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Themes/Back.wav"));
    }

    void ThemesMenuLayout::DisposeAudio() {
        pu::audio::DestroySfx(this->theme_change_sfx);
        pu::audio::DestroySfx(this->back_sfx);
    }

    void ThemesMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(keys_down & HidNpadButton_B) {
            pu::audio::PlaySfx(this->back_sfx);

            g_MenuApplication->LoadMenuByType(MenuType::Main);
        }
    }

    bool ThemesMenuLayout::OnHomeButtonPress() {
        pu::audio::PlaySfx(this->back_sfx);

        g_MenuApplication->LoadMenuByType(MenuType::Main);
        return true;
    }

    void ThemesMenuLayout::Reload() {
        this->themes_menu->ClearItems();
        this->loaded_themes.clear();
        
        this->loaded_themes = cfg::FindThemes();
        this->loaded_themes.insert(this->loaded_themes.begin(), cfg::Theme{}); // For the "default theme" entry

        // Move active theme to top
        for(u32 i = 0; i < this->loaded_themes.size(); i++) {
            const auto theme = this->loaded_themes.at(i);
            if(theme.IsSame(g_GlobalSettings.active_theme)) {
                this->loaded_themes.erase(this->loaded_themes.begin() + i);
                this->loaded_themes.insert(this->loaded_themes.begin(), theme);
                break;
            }
        }
        
        for(const auto &theme: this->loaded_themes) {
            if(theme.IsValid()) {
                std::string theme_icon_path;
                const auto rc = cfg::TryCacheLoadThemeIcon(theme, theme_icon_path);
                if(R_FAILED(rc)) {
                    UL_LOG_WARN("Theme '%s' unable to load image: %s", theme.name.c_str(), util::FormatResultDisplay(rc).c_str());
                }

                auto theme_icon = pu::sdl2::TextureHandle::New(pu::ui::render::LoadImage(theme_icon_path));
                this->loaded_theme_icons.push_back(theme_icon);

                auto theme_item = pu::ui::elm::MenuItem::New(theme.manifest.name + " (v" + theme.manifest.release + ", " + theme.manifest.author + ")");
                theme_item->AddOnKey(std::bind(&ThemesMenuLayout::theme_DefaultKey, this));
                theme_item->SetColor(g_MenuApplication->GetTextColor());
                theme_item->SetIcon(theme_icon);
                this->themes_menu->AddItem(theme_item);
            }
            else {
                this->loaded_theme_icons.emplace_back();
                auto theme_reset_item = pu::ui::elm::MenuItem::New(GetLanguageString("theme_reset"));
                theme_reset_item->AddOnKey(std::bind(&ThemesMenuLayout::theme_DefaultKey, this));
                theme_reset_item->SetColor(g_MenuApplication->GetTextColor());
                theme_reset_item->SetIcon(GetLogoTexture());
                this->themes_menu->AddItem(theme_reset_item);
            }
        }

        this->themes_menu->SetSelectedIndex(0);
    }

    void ThemesMenuLayout::theme_DefaultKey() {
        const auto idx = this->themes_menu->GetSelectedIndex();
        const auto &selected_theme = this->loaded_themes.at(idx);
        if(selected_theme.IsValid()) {
            if(selected_theme.IsSame(g_GlobalSettings.active_theme)) {
                g_MenuApplication->ShowNotification(GetLanguageString("theme_active_this"));
            }
            else {
                std::string theme_conf_msg = selected_theme.manifest.name + "\n";
                theme_conf_msg += selected_theme.manifest.description + "\n";
                theme_conf_msg += "(" + selected_theme.manifest.release + ", " + selected_theme.manifest.author + ")\n\n";
                theme_conf_msg += GetLanguageString("theme_set_conf");

                const auto option = g_MenuApplication->DisplayDialog(selected_theme.manifest.name, theme_conf_msg, { GetLanguageString("yes"), GetLanguageString("cancel") }, true, this->loaded_theme_icons.at(idx));
                if(option == 0) {
                    g_GlobalSettings.SetActiveTheme(selected_theme);
                    g_MenuApplication->ShowNotification(GetLanguageString("theme_cache"));

                    pu::audio::PlaySfx(this->theme_change_sfx);
                    g_MenuApplication->ShowNotification(GetLanguageString("theme_changed"));
                    UL_RC_ASSERT(ul::menu::smi::RestartMenu(true));
                    g_MenuApplication->Finalize();
                }
            }
        }
        else {
            if(g_GlobalSettings.active_theme.IsValid()) {
                const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("theme_reset"), GetLanguageString("theme_reset_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                if(option == 0) {
                    g_GlobalSettings.SetActiveTheme({});
                    g_MenuApplication->ShowNotification(GetLanguageString("theme_cache"));

                    pu::audio::PlaySfx(this->theme_change_sfx);
                    g_MenuApplication->ShowNotification(GetLanguageString("theme_changed"));
                    UL_RC_ASSERT(ul::menu::smi::RestartMenu(true));
                    g_MenuApplication->Finalize();
                }
            }
            else {
                g_MenuApplication->ShowNotification(GetLanguageString("theme_no_active"));
            }
        }
    }

}
