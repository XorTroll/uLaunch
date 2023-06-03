#include <ul/menu/ui/ui_QuickMenu.hpp>
#include <ul/menu/ui/ui_Application.hpp>
#include <ul/menu/ui/ui_Util.hpp>

extern ul::menu::ui::Application::Ref g_Application;

namespace ul::menu::ui {

    namespace {

        std::atomic_bool g_HomePressed = false;

    }

    void QuickMenu::OnHomeButtonDetection() {
        g_HomePressed = true;
    }

    QuickMenu::QuickMenu() {
        this->on = false;
        this->bg_alpha = 0;

        this->options_menu = pu::ui::elm::Menu::New(MenuX, MenuY, MenuWidth, pu::ui::Color { 0x57, 0, 0x7f, 0xff }, pu::ui::Color { 0x86, 0x4e, 0xa0, 0xff }, MenuItemHeight, MenuItemsToShow);
        // g_MenuApplication->ApplyConfigForElement("quick_menu", "quick_menu_item", this->options_menu);
        
        const pu::ui::Color text_clr = { 0xff, 0xff, 0xff, 0xff };

        auto help_item = pu::ui::elm::MenuItem::New("Help & information");
        help_item->SetIcon("sdmc:/umad/uitest/help.png");
        help_item->AddOnKey(&ShowHelpDialog);
        help_item->SetColor(text_clr);
        this->options_menu->AddItem(help_item);

        auto power_item = pu::ui::elm::MenuItem::New("Power options");
        power_item->SetIcon("sdmc:/umad/uitest/power.png");
        power_item->AddOnKey(&ShowPowerDialog);
        power_item->SetColor(text_clr);
        this->options_menu->AddItem(power_item);

        auto controller_item = pu::ui::elm::MenuItem::New("Controller options");
        controller_item->SetIcon("sdmc:/umad/uitest/controller.png");
        controller_item->AddOnKey(&ShowControllerSupport);
        controller_item->SetColor(text_clr);
        this->options_menu->AddItem(controller_item);

        auto album_item = pu::ui::elm::MenuItem::New("Open album");
        album_item->SetIcon("sdmc:/umad/uitest/album.png");
        album_item->AddOnKey(&ShowAlbumApplet);
        album_item->SetColor(text_clr);
        this->options_menu->AddItem(album_item);

        auto web_item = pu::ui::elm::MenuItem::New("Open web-page");
        web_item->SetIcon("sdmc:/umad/uitest/web.png");
        web_item->AddOnKey(&ShowWebPage);
        web_item->SetColor(text_clr);
        this->options_menu->AddItem(web_item);

        auto mii_item = pu::ui::elm::MenuItem::New("Open mii maker");
        mii_item->SetIcon("sdmc:/umad/uitest/mii.png");
        mii_item->AddOnKey(&ShowMiiEdit);
        mii_item->SetColor(text_clr);
        this->options_menu->AddItem(mii_item);

        auto user_item = pu::ui::elm::MenuItem::New("User menu");
        user_item->SetIcon("sdmc:/umad/uitest/user.png");
        user_item->AddOnKey(&ShowUserMenu);
        user_item->SetColor(text_clr);
        this->options_menu->AddItem(user_item);

        auto themes_item = pu::ui::elm::MenuItem::New("Themes menu");
        themes_item->SetIcon("sdmc:/umad/uitest/themes.png");
        themes_item->AddOnKey(&ShowThemesMenu);
        themes_item->SetColor(text_clr);
        this->options_menu->AddItem(themes_item);

        auto settings_item = pu::ui::elm::MenuItem::New("Settings menu");
        settings_item->SetIcon("sdmc:/umad/uitest/settings.png");
        settings_item->AddOnKey(&ShowSettingsMenu);
        settings_item->SetColor(text_clr);
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
        if(!this->on) {
            return;
        }

        this->options_menu->OnInput(keys_down, keys_up, keys_held, touch_pos);

        if((keys_down & HidNpadButton_B) || (keys_down & HidNpadButton_A)) {
            // B only valid for toggling off
            // A = something selected in the menu
            this->Toggle();
        }
        else {
            if(g_HomePressed) {
                this->Toggle();
                g_HomePressed = false;
            }
        }
    }

}