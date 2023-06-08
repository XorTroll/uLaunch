#include <ul/menu/ui/ui_LanguagesMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/os/os_System.hpp>
#include <ul/system/system_Message.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::menu::ui::TransitionGuard g_TransitionGuard;
extern ul::cfg::Theme g_Theme;
extern ul::cfg::Config g_Config;

namespace ul::menu::ui {

    LanguagesMenuLayout::LanguagesMenuLayout() {
        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));

        this->info_text = pu::ui::elm::TextBlock::New(0, 100, GetLanguageString("lang_info_text"));
        this->info_text->SetColor(g_MenuApplication->GetTextColor());
        this->info_text->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        g_MenuApplication->ApplyConfigForElement("languages_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->langs_menu = pu::ui::elm::Menu::New(200, 160, 880, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), 100, 4);
        g_MenuApplication->ApplyConfigForElement("languages_menu", "languages_menu_item", this->langs_menu);
        this->Add(this->langs_menu);
    }

    void LanguagesMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(keys_down & HidNpadButton_B) {
            g_TransitionGuard.Run([]() {
                g_MenuApplication->FadeOut();
                g_MenuApplication->LoadSettingsMenu();
                g_MenuApplication->FadeIn();
            });
        }
    }

    bool LanguagesMenuLayout::OnHomeButtonPress() {
        return g_TransitionGuard.Run([]() {
            g_MenuApplication->FadeOut();
            g_MenuApplication->LoadMenu();
            g_MenuApplication->FadeIn();
        });
    }

    void LanguagesMenuLayout::Reload() {
        this->langs_menu->ClearItems();

        const auto sys_lang = os::GetSystemLanguage();
        for(u32 i = 0; i < os::LanguageNameCount; i++) {
            std::string name = os::LanguageNameList[i];
            if(static_cast<u32>(sys_lang) == i) {
                name += " " + GetLanguageString("lang_selected");
            }

            auto lang_item = pu::ui::elm::MenuItem::New(name);
            lang_item->SetColor(g_MenuApplication->GetTextColor());
            lang_item->AddOnKey(std::bind(&LanguagesMenuLayout::lang_DefaultKey, this, i));
            this->langs_menu->AddItem(lang_item);
        }

        this->langs_menu->SetSelectedIndex(0);
    }

    void LanguagesMenuLayout::lang_DefaultKey(const u32 idx) {
        // TODO: cache system language...

        const auto sys_lang = os::GetSystemLanguage();
        if(static_cast<u32>(sys_lang) == idx) {
            g_MenuApplication->ShowNotification(GetLanguageString("lang_active_this"));
        }
        else {
            const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("lang_set"), GetLanguageString("lang_set_conf"), { GetLanguageString("yes"), GetLanguageString("no") }, true);
            if(option == 0) {
                u64 lang_codes[os::LanguageNameCount] = {};
                s32 tmp;
                setGetAvailableLanguageCodes(&tmp, lang_codes, os::LanguageNameCount);
                const auto lang_code = lang_codes[this->langs_menu->GetSelectedIndex()];

                const auto rc = setsysSetLanguageCode(lang_code);
                g_MenuApplication->CreateShowDialog(GetLanguageString("lang_set"), R_SUCCEEDED(rc) ? GetLanguageString("lang_set_ok") : GetLanguageString("lang_set_error") + ": " + util::FormatResultDisplay(rc), { GetLanguageString("ok") }, true);
                if(R_SUCCEEDED(rc)) {
                    g_TransitionGuard.Run([]() {
                        g_MenuApplication->FadeOut();

                        system::PushSimpleSystemAppletMessage(system::GeneralChannelMessage::Unk_Reboot);
                        svcSleepThread(1'500'000'000ul);
                    });
                }
            }
        }
    }

}