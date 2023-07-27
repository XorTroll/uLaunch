#include <ul/man/ui/ui_MainApplication.hpp>

extern ul::man::ui::MainApplication::Ref g_MainApplication;

namespace ul::man::ui {

    void MainApplication::OnLoad() {
        this->main_menu_lyt = MainMenuLayout::New();
        this->toast = pu::ui::extras::Toast::New("...", pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium), pu::ui::Color(225, 225, 225, 255), pu::ui::Color(40, 40, 40, 255));

        this->SetOnInput(std::bind(&MainApplication::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        this->LoadLayout(this->main_menu_lyt);
    }

    void MainApplication::ShowNotification(const std::string &text) {
        this->EndOverlay();
        this->toast->SetText(text);
        this->StartOverlayWithTimeout(this->toast, 1500);
    }

    void MainApplication::OnInput(const u64 down, const u64 up, const u64 held) {
        if(down & HidNpadButton_Plus) {
            this->CloseWithFadeOut();
        }
    }

}