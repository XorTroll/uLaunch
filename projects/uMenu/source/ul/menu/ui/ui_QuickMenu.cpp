#include <ul/menu/ui/ui_QuickMenu.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/acc/acc_Accounts.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    namespace {

        std::atomic_bool g_HomePressed = false;

    }

    void QuickMenu::OnHomeButtonDetection(const smi::MenuMessageContext _) {
        g_HomePressed = true;
    }

    QuickMenu::QuickMenu() {
        this->on = false;
        this->bg_alpha = 0;

        this->options_menu = pu::ui::elm::Menu::New(MenuX, MenuY, MenuWidth, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), MenuItemHeight, MenuItemsToShow);
        g_MenuApplication->ApplyConfigForElement("quick_menu", "quick_menu", this->options_menu);

        this->power_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_power_options"));
        this->power_menu_item->SetIcon(TryFindLoadImageHandle("ui/PowerQuickIcon"));
        this->power_menu_item->AddOnKey(&ShowPowerDialog);
        this->power_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->power_menu_item);

        this->controller_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_controller_options"));
        this->controller_menu_item->SetIcon(TryFindLoadImageHandle("ui/ControllersQuickIcon"));
        this->controller_menu_item->AddOnKey(&ShowControllerSupport);
        this->controller_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->controller_menu_item);

        this->album_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_album"));
        this->album_menu_item->SetIcon(TryFindLoadImageHandle("ui/AlbumQuickIcon"));
        this->album_menu_item->AddOnKey(&ShowAlbum);
        this->album_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->album_menu_item);

        this->web_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_web_page"));
        this->web_menu_item->SetIcon(TryFindLoadImageHandle("ui/WebBrowserQuickIcon"));
        this->web_menu_item->AddOnKey(&ShowWebPage);
        this->web_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->web_menu_item);

        this->user_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_user_menu"));
        this->user_menu_item->SetIcon(nullptr); // Will be later set, when a user is actually selected
        this->user_menu_item->AddOnKey(&ShowUserPage);
        this->user_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->user_menu_item);

        this->themes_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_themes_menu"));
        this->themes_menu_item->SetIcon(TryFindLoadImageHandle("ui/ThemesQuickIcon"));
        this->themes_menu_item->AddOnKey(&ShowThemesMenu);
        this->themes_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->themes_menu_item);

        this->settings_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_settings_menu"));
        this->settings_menu_item->SetIcon(TryFindLoadImageHandle("ui/SettingsQuickIcon"));
        this->settings_menu_item->AddOnKey(&ShowSettingsMenu);
        this->settings_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->settings_menu_item);

        this->mii_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("quick_mii_edit"));
        this->mii_menu_item->SetIcon(TryFindLoadImageHandle("ui/MiiEditQuickIcon"));
        this->mii_menu_item->AddOnKey(&ShowMiiEdit);
        this->mii_menu_item->SetColor(g_MenuApplication->GetTextColor());
        this->options_menu->AddItem(this->mii_menu_item);
    }

    void QuickMenu::UpdateItems() {
        this->user_menu_item->SetIcon(GetSelectedUserIconTexture());
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

        if((keys_down & HidNpadButton_ZL) || (keys_down & HidNpadButton_ZR)) {
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
