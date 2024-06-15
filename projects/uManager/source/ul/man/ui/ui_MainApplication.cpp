#include <ul/man/ui/ui_MainApplication.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/util/util_Json.hpp>

extern ul::man::ui::MainApplication::Ref g_MainApplication;
ul::util::JSON g_DefaultLanguage;
ul::util::JSON g_MainLanguage;

namespace ul::man::ui {

    void MainApplication::OnLoad() {
        UL_RC_ASSERT(setInitialize());
        cfg::LoadLanguageJsons(ManagerLanguagesPath, g_MainLanguage, g_DefaultLanguage);
        setExit();

        this->main_menu_lyt = MainMenuLayout::New();

        auto toast_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        toast_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        toast_text->SetColor(pu::ui::Color(225, 225, 225, 255));
        this->toast = pu::ui::extras::Toast::New(toast_text, pu::ui::Color(40, 40, 40, 255));

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