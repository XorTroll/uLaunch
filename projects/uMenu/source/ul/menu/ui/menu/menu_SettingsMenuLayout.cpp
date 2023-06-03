#include <ul/menu/ui/menu/menu_SettingsMenuLayout.hpp>
#include <ul/menu/ui/ui_Application.hpp>
#include <ul/menu/ui/ui_Util.hpp>
#include <ul/cfg/cfg_Config.hpp>

extern ul::menu::ui::Application::Ref g_Application;
// extern cfg::Theme g_Theme;
extern ul::cfg::Config g_Config;

namespace ul::menu::ui::menu {

    SettingsMenuLayout::SettingsMenuLayout() {
        this->SetBackgroundImage("sdmc:/umad/uitest/bg.png");

        this->info_text = pu::ui::elm::TextBlock::New(0, 25, "Settings yay");
        // this->info_text->SetColor(g_Application->GetTextColor());
        this->info_text->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        // g_Application->ApplyConfigForElement("settings_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->input_bar = InputBar::New(pu::ui::render::ScreenHeight - 50);
        this->input_bar->AddSetInput(HidNpadButton_B, "Back");
        this->input_bar->AddSetInput(HidNpadButton_A, "Select");
        this->Add(this->input_bar);

        this->settings_menu = pu::ui::elm::Menu::New(50, 80, 1180, pu::ui::Color { 0x57, 0, 0x7f, 0xff }, pu::ui::Color { 0x86, 0x4e, 0xa0, 0xff }, 100, 6);
        // g_Application->ApplyConfigForElement("settings_menu", "settings_menu_item", this->settings_menu);
        
        this->entries.emplace_back("Console nickname",
        []() -> std::string {
            SetSysDeviceNickName nickname = {};
            UL_RC_ASSERT(setsysGetDeviceNickname(&nickname));
            return nickname.nickname;
        },
        []() -> bool {
            SwkbdConfig swkbd;
            UL_RC_ASSERT(swkbdCreate(&swkbd, 0));

            swkbdConfigSetGuideText(&swkbd, "New nickname...");
            SetSysDeviceNickName cur_nickname = {};
            UL_RC_ASSERT(setsysGetDeviceNickname(&cur_nickname));
            swkbdConfigSetInitialText(&swkbd, cur_nickname.nickname);
            swkbdConfigSetStringLenMax(&swkbd, sizeof(cur_nickname.nickname));

            SetSysDeviceNickName new_nickname = {};
            // Note: this result may fail if the user cancels on the keyboard (thus no assert is used)
            const auto rc = swkbdShow(&swkbd, new_nickname.nickname, sizeof(new_nickname.nickname));
            swkbdClose(&swkbd);

            if(R_SUCCEEDED(rc)) {
                UL_RC_ASSERT(setsysSetDeviceNickname(&new_nickname));
                return true;
            }
            else {
                return false;
            }
        });

        this->entries.emplace_back("Connection settings",
        []() -> std::string { return ""; },
        []() -> bool {
            u8 in[28] = {0};
            // 0 = normal, 1 = qlaunch, 2 = starter...?
            *reinterpret_cast<u32*>(in) = 1;

            LibAppletArgs args;
            libappletArgsCreate(&args, 0);

            u8 out[8] = {0};
            const auto rc = libappletLaunch(AppletId_LibraryAppletNetConnect, &args, in, sizeof(in), out, sizeof(out), nullptr);
            return R_SUCCEEDED(rc) && R_SUCCEEDED(*reinterpret_cast<Result*>(out));
        });

        /*
        LISTA:

        * Nombre consola
        - Modo avion?
        - amiibos?
        - audio blutuch?
        - salida tv (resolucion tv, rango rgb, ...)
        - cosas mandos
        - brillo?
        - bloqueo pantalla!
        - pctl?
        - gestion de datos? mejor gleaf...
        - wifi!
        - mostrar % bateria
        - ajustes consola, etc.

        */

        this->ReloadEntries(true);
        this->Add(this->settings_menu);
    }

    void SettingsMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(keys_down & HidNpadButton_B) {
            ShowMainMenu();
        }
    }

    bool SettingsMenuLayout::OnHomeButtonPress() {
        ShowMainMenu();
        return true;
    }

    void SettingsMenuLayout::entry_DefaultKey(const u32 entry_idx) {
        const auto &entry = this->entries.at(entry_idx);

        const auto needs_reload = entry.Select();
        if(needs_reload) {
            g_Application->FadeOut();
            this->ReloadEntries(false);
            g_Application->FadeIn();
        }
    }

    void SettingsMenuLayout::ReloadEntries(const bool rewind_menu) {
        this->settings_menu->ClearItems();

        for(u32 i = 0; i < this->entries.size(); i++) {
            const auto &entry = this->entries.at(i);
            auto entry_item = pu::ui::elm::MenuItem::New(entry.GetDescription());

            entry_item->AddOnKey(std::bind(&SettingsMenuLayout::entry_DefaultKey, this, i));
            entry_item->SetIcon("sdmc:/umad/uitest/setting.png");
            this->settings_menu->AddItem(entry_item);
        }
    }

}