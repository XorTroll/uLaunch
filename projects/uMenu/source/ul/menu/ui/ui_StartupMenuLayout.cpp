#include <ul/menu/ui/ui_StartupMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/menu/smi/smi_Commands.hpp>

extern ul::menu::ui::GlobalSettings g_GlobalSettings;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    void StartupMenuLayout::user_DefaultKey(const AccountUid uid) {
        // Note: menu loading is invoked below instead of here so that the main menu doesn't also register the button input which caused this action...
        this->load_menu = true;
        pu::audio::PlaySfx(this->user_select_sfx);
        g_GlobalSettings.SetSelectedUser(uid);

        g_MenuApplication->LoadMenu(MenuType::Main);
    }

    void StartupMenuLayout::create_DefaultKey() {
        pu::audio::PlaySfx(this->user_create_sfx);
        UL_RC_ASSERT(smi::OpenAddUser());

        g_MenuApplication->Finalize();
    }

    StartupMenuLayout::StartupMenuLayout() : IMenuLayout() {
        this->load_menu = false;

        this->user_create_sfx = nullptr;
        this->user_select_sfx = nullptr;

        this->info_text = pu::ui::elm::TextBlock::New(0, 0, GetLanguageString("startup_welcome_info"));
        this->info_text->SetColor(g_MenuApplication->GetTextColor());
        g_GlobalSettings.ApplyConfigForElement("startup_menu", "info_text", this->info_text);
        this->Add(this->info_text);

        this->users_menu = pu::ui::elm::Menu::New(0, 0, UsersMenuWidth, g_MenuApplication->GetMenuBackgroundColor(), g_MenuApplication->GetMenuFocusColor(), UsersMenuItemSize, UsersMenuItemsToShow);
        g_GlobalSettings.ApplyConfigForElement("startup_menu", "users_menu", this->users_menu);
        this->Add(this->users_menu);
    }

    void StartupMenuLayout::LoadSfx() {
        this->user_create_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Startup/UserCreate.wav"));
        this->user_select_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Startup/UserSelect.wav"));
    }
    
    void StartupMenuLayout::DisposeSfx() {
        pu::audio::DestroySfx(this->user_create_sfx);
        pu::audio::DestroySfx(this->user_select_sfx);
    }

    void StartupMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        // ...
    }

    bool StartupMenuLayout::OnHomeButtonPress() {
        // ...
        return true;
    }

    void StartupMenuLayout::ReloadMenu() {
        this->users_menu->ClearItems();

        std::vector<AccountUid> users;
        if(R_SUCCEEDED(acc::ListAccounts(users))) {
            for(const auto &user: users) {
                std::string name;
                if(R_SUCCEEDED(acc::GetAccountName(user, name))) {
                    auto user_item = pu::ui::elm::MenuItem::New(name);
                    auto user_icon = pu::sdl2::TextureHandle::New(pu::ui::render::LoadImage(acc::GetIconCacheImagePath(user)));
                    user_item->SetIcon(user_icon);
                    user_item->AddOnKey(std::bind(&StartupMenuLayout::user_DefaultKey, this, user));
                    user_item->SetColor(g_MenuApplication->GetTextColor());
                    this->users_menu->AddItem(user_item);
                }
            }
        }

        auto create_user_item = pu::ui::elm::MenuItem::New(GetLanguageString("startup_add_user"));
        create_user_item->SetColor(g_MenuApplication->GetTextColor());
        create_user_item->AddOnKey(std::bind(&StartupMenuLayout::create_DefaultKey, this));
        this->users_menu->AddItem(create_user_item);

        this->users_menu->SetSelectedIndex(0);
    }

}
