#include <ui/ui_LanguagesMenuLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <fs/fs_Stdio.hpp>
#include <os/os_Misc.hpp>
#include <os/os_HomeMenu.hpp>
#include <net/net_Service.hpp>
#include <am/am_LibraryApplet.hpp>

extern ui::MenuApplication::Ref g_MenuApplication;
extern cfg::Theme g_Theme;
extern cfg::Config g_Config;

namespace ui {

    LanguagesMenuLayout::LanguagesMenuLayout() {
        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));

        pu::ui::Color textclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        pu::ui::Color menufocusclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        pu::ui::Color menubgclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->infoText = pu::ui::elm::TextBlock::New(0, 100, cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "lang_info_text"));
        this->infoText->SetColor(textclr);
        this->infoText->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        g_MenuApplication->ApplyConfigForElement("languages_menu", "info_text", this->infoText);
        this->Add(this->infoText);

        this->langsMenu = pu::ui::elm::Menu::New(200, 160, 880, menubgclr, 100, 4);
        this->langsMenu->SetOnFocusColor(menufocusclr);
        g_MenuApplication->ApplyConfigForElement("languages_menu", "languages_menu_item", this->langsMenu);
        this->Add(this->langsMenu);
    }

    void LanguagesMenuLayout::OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) {
        if(down & KEY_B) {
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadSettingsMenu();
            g_MenuApplication->FadeIn();
        }
    }

    void LanguagesMenuLayout::OnHomeButtonPress() {
        g_MenuApplication->FadeOut();
        g_MenuApplication->LoadMenu();
        g_MenuApplication->FadeIn();
    }

    void LanguagesMenuLayout::Reload() {
        this->langsMenu->ClearItems();
        this->langsMenu->SetSelectedIndex(0);

        auto textclr = pu::ui::Color::FromHex(g_MenuApplication->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        u64 lcode = 0;
        auto ilang = SetLanguage_ENUS;
        setGetLanguageCode(&lcode);
        setMakeLanguage(lcode, &ilang);
        
        u32 idx = 0;
        for(auto &lang: os::GetLanguageNameList()) {
            auto name = lang;
            if(static_cast<u32>(ilang) == idx) {
                name += " " + cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "lang_selected");
            }
            auto litm = pu::ui::elm::MenuItem::New(name);
            litm->SetColor(textclr);
            litm->AddOnClick(std::bind(&LanguagesMenuLayout::lang_Click, this, idx));
            this->langsMenu->AddItem(litm);
            idx++;
        }
    }

    void LanguagesMenuLayout::lang_Click(u32 idx) {
        u64 lcode = 0;
        SetLanguage ilang = SetLanguage_ENUS;
        setGetLanguageCode(&lcode);
        setMakeLanguage(lcode, &ilang);

        if(static_cast<u32>(ilang) == idx) {
            g_MenuApplication->ShowNotification(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "lang_active_this"));
        }
        else {
            auto sopt = g_MenuApplication->CreateShowDialog(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "lang_set"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "lang_set_conf"), { cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "yes"), cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "no") }, true);
            if(sopt == 0) {
                u64 codes[16] = {0};
                s32 tmp;
                setGetAvailableLanguageCodes(&tmp, codes, 16);
                auto code = codes[this->langsMenu->GetSelectedIndex()];

                auto rc = setsysSetLanguageCode(code);
                g_MenuApplication->CreateShowDialog(cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "lang_set"), R_SUCCEEDED(rc) ? cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "lang_set_ok") : cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "lang_set_error") + ": " + util::FormatResult(rc), { cfg::GetLanguageString(g_Config.main_lang, g_Config.default_lang, "ok") }, true);
                if(R_SUCCEEDED(rc)) {
                    g_MenuApplication->FadeOut();

                    auto smsg = os::SystemAppletMessage::Create(os::GeneralChannelMessage::Reboot);
                    os::PushSystemAppletMessage(smsg);
                    svcSleepThread(1'500'000'000ul);
                }
            }
        }
    }

}