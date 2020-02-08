#include <ui/ui_QuickMenu.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <cfg/cfg_Config.hpp>

extern cfg::Theme g_ul_theme;
extern ui::MenuApplication::Ref g_menu_app_instance;

namespace ui
{
    QuickMenu::QuickMenu(const std::string &main_icon)
    {
        this->on = false;
        this->bgalpha = 0;

        pu::ui::Color menufocusclr = pu::ui::Color::FromHex(g_menu_app_instance->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        pu::ui::Color menubgclr = pu::ui::Color::FromHex(g_menu_app_instance->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->dev_opt_menu = pu::ui::elm::Menu::New(200, 160, 880, pu::ui::Color{ 0xff, 0xff, 0xff, 0x10 }, 100, 4);
        this->dev_opt_menu->SetOnFocusColor(menufocusclr);
        this->dev_opt_menu->SetColor({ 0, 0, 0, 0 });
        g_menu_app_instance->ApplyConfigForElement("quick_menu", "quick_menu_item", this->dev_opt_menu);
        
        auto controller_item = pu::ui::elm::MenuItem::New("Controller");
        controller_item->SetIcon(cfg::GetAssetByTheme(g_ul_theme, "ui/ControllerIcon.png"));
        controller_item->AddOnClick(&actions::ShowControllerSupport);
        this->dev_opt_menu->AddItem(controller_item);
    }

    s32 QuickMenu::GetX()
    {
        return 0;
    }

    s32 QuickMenu::GetY()
    {
        return 0;
    }

    s32 QuickMenu::GetWidth()
    {
        return 1280;
    }

    s32 QuickMenu::GetHeight()
    {
        return 720;
    }

    void QuickMenu::Toggle()
    {
        this->on = !this->on;
    }
    
    bool QuickMenu::IsOn()
    {
        return this->on && (this->bgalpha > 0);
    }

    void QuickMenu::OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y)
    {
        if(!this->on)
        {
            if(this->bgalpha > 0)
            {
                this->bgalpha -= 20;
                if(this->bgalpha < 0) this->bgalpha = 0;
            }
        }
        else
        {
            if(this->bgalpha < 220)
            {
                this->bgalpha += 20;
                if(this->bgalpha > 220) this->bgalpha = 220;
            }
        }
        this->dev_opt_menu->SetVisible(this->bgalpha != 0);

        Drawer->RenderRectangleFill({ 50, 50, 50, (u8)this->bgalpha }, 0, 0, 1280, 720);

        if(this->bgalpha > 0) {
            Drawer->SetBaseRenderAlpha((u8)this->bgalpha);
            this->dev_opt_menu->OnRender(Drawer, this->dev_opt_menu->GetProcessedX(), this->dev_opt_menu->GetProcessedY());
            Drawer->UnsetBaseRenderAlpha();
        }
    }

    void QuickMenu::OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos)
    {
        if(this->on) {
            this->dev_opt_menu->OnInput(Down, Up, Held, Pos);
        }

        if((Down & KEY_L) || (Down & KEY_R) || (Down & KEY_ZL) || (Down & KEY_ZR)) {
            this->Toggle();
        }
        else if(Down & KEY_B) {
            // B only valid for toggling off
            if(this->on) {
                this->Toggle();
            }
        }
    }
}