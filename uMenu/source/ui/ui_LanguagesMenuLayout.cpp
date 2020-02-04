#include <ui/ui_LanguagesMenuLayout.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <fs/fs_Stdio.hpp>
#include <os/os_Misc.hpp>
#include <os/os_HomeMenu.hpp>
#include <net/net_Service.hpp>
#include <am/am_LibraryApplet.hpp>

extern ui::MenuApplication::Ref qapp;
extern cfg::Theme theme;
extern cfg::Config config;

namespace ui
{
    LanguagesMenuLayout::LanguagesMenuLayout()
    {
        this->SetBackgroundImage(cfg::GetAssetByTheme(theme, "ui/Background.png"));

        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        pu::ui::Color menufocusclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_focus_color", "#5ebcffff"));
        pu::ui::Color menubgclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("menu_bg_color", "#0094ffff"));

        this->infoText = pu::ui::elm::TextBlock::New(0, 100, cfg::GetLanguageString(config.main_lang, config.default_lang, "lang_info_text"));
        this->infoText->SetColor(textclr);
        this->infoText->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        qapp->ApplyConfigForElement("languages_menu", "info_text", this->infoText);
        this->Add(this->infoText);

        this->langsMenu = pu::ui::elm::Menu::New(200, 160, 880, menubgclr, 100, 4);
        this->langsMenu->SetOnFocusColor(menufocusclr);
        qapp->ApplyConfigForElement("languages_menu", "languages_menu_item", this->langsMenu);
        this->Add(this->langsMenu);

        this->SetOnInput(std::bind(&LanguagesMenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void LanguagesMenuLayout::Reload()
    {
        this->langsMenu->ClearItems();
        this->langsMenu->SetSelectedIndex(0);

        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        u64 lcode = 0;
        SetLanguage ilang = SetLanguage_ENUS;
        setGetLanguageCode(&lcode);
        setMakeLanguage(lcode, &ilang);
        
        u32 idx = 0;
        for(auto &lang: os::LanguageNames)
        {
            auto name = lang;
            if((u32)ilang == idx) name += " " + cfg::GetLanguageString(config.main_lang, config.default_lang, "lang_selected");
            auto litm = pu::ui::elm::MenuItem::New(name);
            litm->SetColor(textclr);
            litm->AddOnClick(std::bind(&LanguagesMenuLayout::lang_Click, this, idx));
            this->langsMenu->AddItem(litm);
            idx++;
        }
    }

    void LanguagesMenuLayout::lang_Click(u32 idx)
    {
        u64 lcode = 0;
        SetLanguage ilang = SetLanguage_ENUS;
        setGetLanguageCode(&lcode);
        setMakeLanguage(lcode, &ilang);

        if((u32)ilang == idx) qapp->ShowNotification(cfg::GetLanguageString(config.main_lang, config.default_lang, "lang_active_this"));
        else
        {
            auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "lang_set"), cfg::GetLanguageString(config.main_lang, config.default_lang, "lang_set_conf"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "no") }, true);
            if(sopt == 0)
            {
                u64 codes[16] = {0};
                s32 tmp;
                setGetAvailableLanguageCodes(&tmp, codes, 16);
                u64 code = codes[this->langsMenu->GetSelectedIndex()];

                auto rc = setsysSetLanguageCode(code);
                qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "lang_set"), R_SUCCEEDED(rc) ? cfg::GetLanguageString(config.main_lang, config.default_lang, "lang_set_ok") : cfg::GetLanguageString(config.main_lang, config.default_lang, "lang_set_error") + ": " + util::FormatResult(rc), { cfg::GetLanguageString(config.main_lang, config.default_lang, "ok") }, true);
                if(R_SUCCEEDED(rc))
                {
                    qapp->FadeOut();
                    os::SystemAppletMessage smsg = {};
                    smsg.magic = os::SAMSMagic;
                    smsg.message = (u32)os::GeneralChannelMessage::Reboot;

                    os::PushSystemAppletMessage(smsg);
                    svcSleepThread(1'500'000'000L);
                }
            }
        }
    }

    void LanguagesMenuLayout::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos)
    {
        qapp->CommonMenuOnLoop();

        bool ret = am::MenuIsHomePressed();
        if(down & KEY_B)
        {
            qapp->FadeOut();
            qapp->LoadSettingsMenu();
            qapp->FadeIn();
        }
        if(ret)
        {
            qapp->FadeOut();
            qapp->LoadMenu();
            qapp->FadeIn();
        }
    }
}