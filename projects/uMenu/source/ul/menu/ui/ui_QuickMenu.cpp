#include <ul/menu/ui/ui_QuickMenu.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/cfg/cfg_Config.hpp>

extern ul::cfg::Theme g_Theme;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    namespace {

        std::atomic_bool g_HomePressed = false;

    }

    void QuickMenu::OnHomeButtonDetection(const smi::MenuMessageContext _) {
        g_HomePressed = true;
    }

    QuickMenu::QuickMenu(const std::string &main_icon) {
        this->on = false;
        this->bg_alpha = 0;

        this->options_menu = pu::ui::elm::Menu::New(MenuX, MenuY, MenuWidth, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), MenuItemHeight, MenuItemsToShow);
        g_MenuApplication->ApplyConfigForElement("quick_menu", "quick_menu_item", this->options_menu);

        auto power_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_power_options"));
        power_item->SetIcon(TryFindImage(g_Theme, "ui/PowerIcon"));
        power_item->AddOnKey(&ShowPowerDialog);
        power_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(power_item);

        auto controller_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_controller_options"));
        controller_item->SetIcon(TryFindImage(g_Theme, "ui/ControllerIcon"));
        controller_item->AddOnKey(&ShowControllerSupport);
        controller_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(controller_item);

        auto album_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_album"));
        album_item->SetIcon(TryFindImage(g_Theme, "ui/AlbumIcon"));
        album_item->AddOnKey(&ShowAlbumApplet);
        album_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(album_item);

        auto web_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_web_page"));
        web_item->SetIcon(TryFindImage(g_Theme, "ui/WebIcon"));
        web_item->AddOnKey(&ShowWebPage);
        web_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(web_item);

        auto user_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_user_menu"));
        user_item->SetIcon(TryFindImage(g_Theme, "ui/UserIcon"));
        user_item->AddOnKey(&ShowUserMenu);
        user_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(user_item);

        auto themes_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_themes_menu"));
        themes_item->SetIcon(TryFindImage(g_Theme, "ui/ThemesIcon"));
        themes_item->AddOnKey(&ShowThemesMenu);
        themes_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(themes_item);

        auto settings_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_settings_menu"));
        settings_item->SetIcon(TryFindImage(g_Theme, "ui/SettingsIcon"));
        settings_item->AddOnKey(&ShowSettingsMenu);
        settings_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(settings_item);
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
            this->Toggle();
        }
        else if((keys_down & HidNpadButton_B) || (keys_down & HidNpadButton_A)) {
            // B only valid for toggling off
            // A = something selected in the menu
            if(this->on) {
                this->Toggle();
            }
        }
        else {
            if(g_HomePressed) {
                if(this->on) {
                    this->Toggle();
                }
                
                g_HomePressed = false;
            }
        }
    }

}