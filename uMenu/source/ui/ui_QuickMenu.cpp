#include <ui/ui_QuickMenu.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <cfg/cfg_Config.hpp>
#include <am/am_DaemonMessages.hpp>

extern cfg::Theme g_Theme;
extern ui::MenuApplication::Ref g_MenuApplication;

namespace ui {

    namespace {

        std::atomic_bool g_HomePressed = false;

    }

    void QuickMenu::OnHomeButtonDetection() {
        g_HomePressed = true;
    }

    QuickMenu::QuickMenu(const std::string &main_icon) {
        this->on = false;
        this->bg_alpha = 0;

        this->options_menu = pu::ui::elm::Menu::New(MenuX, MenuY, MenuWidth, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), MenuItemHeight, MenuItemsToShow);
        g_MenuApplication->ApplyConfigForElement("quick_menu", "quick_menu_item", this->options_menu);
        
        auto help_item = pu::ui::elm::MenuItem::New("Help & information");
        help_item->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/HelpIcon.png"));
        help_item->AddOnKey(&actions::ShowHelpDialog);
        help_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(help_item);

        auto power_item = pu::ui::elm::MenuItem::New("Power options");
        power_item->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/PowerIcon.png"));
        power_item->AddOnKey(&actions::ShowPowerDialog);
        power_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(power_item);

        auto controller_item = pu::ui::elm::MenuItem::New("Controller options");
        controller_item->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/ControllerIcon.png"));
        controller_item->AddOnKey(&actions::ShowControllerSupport);
        controller_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(controller_item);

        auto album_item = pu::ui::elm::MenuItem::New("Open album");
        album_item->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/AlbumIcon.png"));
        album_item->AddOnKey(&actions::ShowAlbumApplet);
        album_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(album_item);

        auto web_item = pu::ui::elm::MenuItem::New("Open web-page");
        web_item->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/WebIcon.png"));
        web_item->AddOnKey(&actions::ShowWebPage);
        web_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(web_item);

        auto user_item = pu::ui::elm::MenuItem::New("User menu");
        user_item->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/UserIcon.png"));
        user_item->AddOnKey(&actions::ShowUserMenu);
        user_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(user_item);

        auto themes_item = pu::ui::elm::MenuItem::New("Themes menu");
        themes_item->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/ThemesIcon.png"));
        themes_item->AddOnKey(&actions::ShowThemesMenu);
        themes_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(themes_item);

        auto settings_item = pu::ui::elm::MenuItem::New("Settings menu");
        settings_item->SetIcon(cfg::GetAssetByTheme(g_Theme, "ui/SettingsIcon.png"));
        settings_item->AddOnKey(&actions::ShowSettingsMenu);
        settings_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(settings_item);

        this->menu_open_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/QuickMenuOpen.wav"));
        this->menu_close_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/QuickMenuClose.wav"));
        this->menu_scroll_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/QuickMenuScroll.wav"));

    }

    void QuickMenu::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        if(!this->on) {
            if(this->bg_alpha > 0) {
                this->bg_alpha -= BackgroundAlphaIncrement;
                if(this->bg_alpha < 0) {
                    this->bg_alpha = 0;
                }
            }
        }
        else {
            if(this->bg_alpha < BackgroundAlphaMax) {
                this->bg_alpha += BackgroundAlphaIncrement;
                if(this->bg_alpha > BackgroundAlphaMax) {
                    this->bg_alpha = BackgroundAlphaMax;
                }
            }
        }
        this->options_menu->SetVisible(this->bg_alpha != 0);

        drawer->RenderRectangleFill(MakeBackgroundColor(static_cast<u8>(this->bg_alpha)), this->GetX(), this->GetY(), this->GetWidth(), this->GetHeight());

        if(this->bg_alpha > 0) {
            if(this->bg_alpha < BackgroundAlphaMax) {
                drawer->SetBaseRenderAlpha(static_cast<u8>(this->bg_alpha));
            }
            this->options_menu->OnRender(drawer, this->options_menu->GetProcessedX(), this->options_menu->GetProcessedY());
            if(this->bg_alpha < BackgroundAlphaMax) {
                drawer->ResetBaseRenderAlpha();
            }
        }
    }

    void QuickMenu::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->on) {
            this->options_menu->OnInput(keys_down, keys_up, keys_held, touch_pos);
        }
        if((keys_down & HidNpadButton_L) || (keys_down & HidNpadButton_R) || (keys_down & HidNpadButton_ZL) || (keys_down & HidNpadButton_ZR)) {
            if(!this->on){ //Play menu opening
                pu::audio::PlaySfx(this->menu_open_sfx);
            }else{ //Play menu closing if i am closing the menu with L R ZL ZR
                pu::audio::PlaySfx(this->menu_close_sfx);
            }
            this->Toggle();
            
        }
        else if((keys_down & HidNpadButton_B) || (keys_down & HidNpadButton_A)) {
            // B only valid for toggling off
            // A = something selected in the menu
            if(this->on) {
                if((keys_down & HidNpadButton_B)){ //Pressing B means closing the menu
                    pu::audio::PlaySfx(this->menu_close_sfx);
                }
                this->Toggle();
            }
        }else if((keys_down & HidNpadButton_AnyUp || keys_down & HidNpadButton_AnyDown)){
            pu::audio::PlaySfx(this->menu_scroll_sfx); //Playing sfx on menu scroll
        }
        else {
            if(g_HomePressed) { //The input is the home button
                if(this->on) {
                    pu::audio::PlaySfx(this->menu_close_sfx); //Plays the close sfx if home button is pressed and menu is open
                    this->Toggle();
                }
                g_HomePressed = false;
            }
        }
    }

}