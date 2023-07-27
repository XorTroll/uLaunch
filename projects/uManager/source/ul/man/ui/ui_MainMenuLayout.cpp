#include <ul/man/ui/ui_MainApplication.hpp>
#include <ul/man/man_Manager.hpp>

extern ul::man::ui::MainApplication::Ref g_MainApplication;

namespace ul::man::ui {

    namespace {

        inline std::string GetStatus() {
            std::string status = "Status: ";
            if(IsBasePresent()) {
                if(IsSystemActive()) {
                    status += "active";
                }
                else {
                    status += "not active";
                }
            }
            else {
                status += "not present";
            }

            return status;
        }

    }

    MainMenuLayout::MainMenuLayout() : pu::ui::Layout() {
        this->info_text = pu::ui::elm::TextBlock::New(0, 25, "uManager - uLaunch's manager, v" UL_VERSION);
        this->info_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge));
        this->info_text->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->info_text->SetColor(pu::ui::Color(0, 0, 0, 255));
        this->Add(this->info_text);

        this->options_menu = pu::ui::elm::Menu::New(0, 70, pu::ui::render::ScreenWidth, pu::ui::Color(80, 0, 120, 255), pu::ui::Color(127, 0, 190, 255), (pu::ui::render::ScreenHeight - 100) / MenuItemCount, MenuItemCount);
        this->activate_menu_item = pu::ui::elm::MenuItem::New(GetStatus());
        this->activate_menu_item->SetColor(pu::ui::Color(225, 225, 225, 255));
        this->activate_menu_item->AddOnKey(std::bind(&MainMenuLayout::activate_DefaultKey, this));
        this->options_menu->AddItem(this->activate_menu_item);
        this->Add(this->options_menu);
        this->SetBackgroundColor(pu::ui::Color(192, 128, 217, 255));
    }

    void MainMenuLayout::activate_DefaultKey() {
        if(IsBasePresent()) {
            if(IsSystemActive()) {
                DeactivateSystem();
            }
            else {
                ActivateSystem();
            }

            this->activate_menu_item->SetName(GetStatus());
            this->options_menu->ClearItems();
            this->options_menu->AddItem(this->activate_menu_item);

            const auto option = g_MainApplication->CreateShowDialog("Changes", "The (de)activation of uLaunch needs a reboot for changes to take effect", { "Reboot", "Continue" }, true);
            if(option == 0) {
                if(R_SUCCEEDED(spsmInitialize())) {
                    spsmShutdown(true);
                }
            }
        }
        else {
            g_MainApplication->CreateShowDialog("uLaunch", "uLaunch was not found in the SD card", { "Ok" }, true);
        }
    }

}