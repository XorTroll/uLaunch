#include <ul/menu/ui/ui_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/menu/menu_Cache.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/util/util_String.hpp>
#include <ul/net/net_Service.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/os/os_System.hpp>
#include <ul/os/os_Applications.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::cfg::Config g_Config;

namespace ul::menu::ui {

    namespace {

        inline loader::TargetInput CreateLaunchTargetInput(const loader::TargetInput &base_params) {
            loader::TargetInput ipt = {};
            util::CopyToStringBuffer(ipt.nro_path, base_params.nro_path);
            if(strlen(base_params.nro_argv) > 0) {
                const auto default_argv = std::string(base_params.nro_path) + " " + base_params.nro_argv;
                util::CopyToStringBuffer(ipt.nro_argv, default_argv);
            }
            else {
                util::CopyToStringBuffer(ipt.nro_argv, base_params.nro_path);
            }
            return ipt;
        }

        inline bool IsEntryNonRemovable(const Entry &entry) {
            if(strcmp(entry.hb_info.nro_target.nro_path, ul::HbmenuPath) == 0) {
                return true;
            }
            if(strcmp(entry.hb_info.nro_target.nro_path, ul::ManagerPath) == 0) {
                return true;
            }

            if(entry.IsSpecial()) {
                return true;
            }

            return false;
        }

        std::vector<std::string> g_WeekdayList;
        std::string g_UserName;

        char g_MenuFsPathBuffer[FS_MAX_PATH] = {};
        char g_MenuPathBuffer[FS_MAX_PATH] = {};

    }

    void MainMenuLayout::DoMoveTo(const std::string &new_path) {
        // Empty path used as a "reload" argumnet
        if(!new_path.empty()) {
            util::CopyToStringBuffer(g_MenuFsPathBuffer, new_path);
            util::CopyToStringBuffer(g_MenuPathBuffer, this->cur_folder_path);
            UL_RC_ASSERT(smi::UpdateMenuPaths(g_MenuFsPathBuffer, g_MenuPathBuffer));
        }

        this->entry_menu->MoveTo(new_path);
    }

    void MainMenuLayout::menu_EntryInputPressed(const u64 keys_down) {
        if(keys_down & HidNpadButton_B) {
            if(this->entry_menu->IsAnySelected()) {
                pu::audio::PlaySfx(this->entry_cancel_select_sfx);
                
                this->StopSelection();
            }
            else if(this->entry_menu->IsInRoot()) {
                const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("user_logoff"), GetLanguageString("user_logoff_opt"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true );
                if(option == 0) {
                    auto log_off = false;
                    if(g_MenuApplication->IsSuspended()) {
                        const auto option_2 = g_MenuApplication->DisplayDialog(GetLanguageString("suspended_app"), GetLanguageString("user_logoff_app_suspended"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                        if(option_2 == 0) {
                            log_off = true;
                        }
                    }
                    else {
                        log_off = true;
                    }

                    if(log_off) {
                        if(g_MenuApplication->IsSuspended()) {
                            this->DoTerminateApplication();
                        }

                        pu::audio::PlaySfx(this->logoff_sfx);

                        g_MenuApplication->LoadMenuByType(MenuType::Startup, true, [&]() {
                            this->MoveToRoot(false);
                        });
                    }
                }
            }
            else {
                const auto parent_path = fs::GetBaseDirectory(this->entry_menu->GetPath());
                this->PopFolder();
                this->cur_path_text->SetText(this->cur_folder_path);
                this->MoveTo(parent_path, true);
            }
        }
        else if(keys_down & HidNpadButton_A) {
            if(this->entry_menu->IsFocusedNonemptyEntry()) {
                auto &cur_entry = this->entry_menu->GetFocusedEntry();
                if(this->entry_menu->IsAnySelected()) {
                    auto do_swap = true;
                    if(cur_entry.Is<EntryType::Folder>()) {
                        do_swap = false;
                        if(this->entry_menu->IsFocusedEntrySelected()) {
                            g_MenuApplication->ShowNotification(GetLanguageString("menu_move_folder_itself"));
                        }
                        else {
                            const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("menu_selection"), GetLanguageString("menu_move_conf"), { GetLanguageString("menu_move_into_folder"), GetLanguageString("menu_move_swap"), GetLanguageString("cancel") }, true);
                            if(option == 0) {
                                auto &sel_entry = this->entry_menu->GetSelectedEntry();

                                Entry sel_entry_copy(sel_entry);
                                sel_entry_copy.MoveTo(cur_entry.GetFolderPath());
                                pu::audio::PlaySfx(this->entry_move_into_sfx);
                                this->StopSelection();
                                this->entry_menu->NotifyEntryRemoved(sel_entry);
                                this->entry_menu->OrganizeUpdateEntries();
                            }
                            else if(option == 1) {
                                do_swap = true;
                            }
                        }
                    }

                    if(do_swap) {
                        auto &sel_entry = this->entry_menu->GetSelectedEntry();
                        Entry old_cur_entry(cur_entry);
                        Entry old_sel_entry(sel_entry);
                        cur_entry.OrderSwap(sel_entry);
                        pu::audio::PlaySfx(this->entry_swap_sfx);
                        this->StopSelection();
                        this->entry_menu->NotifyEntryRemoved(old_cur_entry);
                        this->entry_menu->NotifyEntryRemoved(old_sel_entry);
                        this->entry_menu->NotifyEntryAdded(cur_entry);
                        this->entry_menu->NotifyEntryAdded(sel_entry);
                        this->entry_menu->OrganizeUpdateEntries();
                    }
                }
                else {
                    if(cur_entry.Is<EntryType::Folder>()) {
                        this->PushFolder(cur_entry.folder_info.name);
                        this->MoveTo(cur_entry.GetFolderPath(), true);
                        this->cur_path_text->SetText(this->cur_folder_path);
                    }
                    else if(cur_entry.Is<EntryType::Application>() || cur_entry.Is<EntryType::Homebrew>()) {
                        auto do_launch_entry = true;

                        if(g_MenuApplication->IsSuspended()) {
                            // Play animations, then resume the suspended hb/app
                            if(g_MenuApplication->IsEntrySuspended(cur_entry)) {
                                if(this->mode == SuspendedImageMode::Focused) {
                                    this->StartResume();
                                }
                                do_launch_entry = false;
                            }

                            // If the suspended entry is another one, ask the user to close it beforehand
                            // Homebrew launching code already does this checks later, this do this check only with apps
                            if(do_launch_entry && cur_entry.Is<EntryType::Application>()) {
                                do_launch_entry = false;
                                this->HandleCloseSuspended();
                                do_launch_entry = !g_MenuApplication->IsSuspended();
                            }
                        }

                        if(do_launch_entry && cur_entry.Is<EntryType::Application>() && !cur_entry.app_info.IsLaunchable()) {
                            UL_LOG_WARN("Tried to launch application with type 0x%X", cur_entry.app_info.record.type);
                            pu::audio::PlaySfx(this->error_sfx);
                            // TODO: gamecard detection?
                            // TODO: support for "fixing" corrupted apps, like regular homemenu?
                            g_MenuApplication->ShowNotification(GetLanguageString("app_not_launchable"));
                            do_launch_entry = false;
                        }

                        if(do_launch_entry) {
                            if(cur_entry.Is<EntryType::Homebrew>()) {
                                this->HandleHomebrewLaunch(cur_entry);
                            }
                            else {
                                pu::audio::PlaySfx(this->launch_app_sfx);

                                const auto rc = smi::LaunchApplication(cur_entry.app_info.record.application_id);
                                if(R_SUCCEEDED(rc)) {
                                    g_MenuApplication->Finalize();
                                    return;
                                }
                                else {
                                    g_MenuApplication->ShowNotification(GetLanguageString("app_launch_error") + ": " + util::FormatResultDisplay(rc));
                                }
                            }
                        }
                    }
                    else {
                        switch(cur_entry.type) {
                            case EntryType::SpecialEntryMiiEdit: {
                                pu::audio::PlaySfx(this->open_mii_edit_sfx);
                                ShowMiiEdit();
                                break;
                            }
                            case EntryType::SpecialEntryWebBrowser: {
                                pu::audio::PlaySfx(this->open_web_browser_sfx);
                                ShowWebPage();
                                break;
                            }
                            case EntryType::SpecialEntryUserPage: {
                                pu::audio::PlaySfx(this->open_user_page_sfx);
                                ShowUserPage();
                                break;
                            }
                            case EntryType::SpecialEntrySettings: {
                                pu::audio::PlaySfx(this->open_settings_sfx);
                                ShowSettingsMenu();
                                break;
                            }
                            case EntryType::SpecialEntryThemes: {
                                pu::audio::PlaySfx(this->open_themes_sfx);
                                ShowThemesMenu();
                                break;
                            }
                            case EntryType::SpecialEntryControllers: {
                                pu::audio::PlaySfx(this->open_controllers_sfx);
                                ShowControllerSupport();
                                break;
                            }
                            case EntryType::SpecialEntryAlbum: {
                                pu::audio::PlaySfx(this->open_album_sfx);
                                ShowAlbum();
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
            }
            else {
                // Move entry to a currently empty position
                if(this->entry_menu->IsAnySelected()) {
                    auto &sel_entry = this->entry_menu->GetSelectedEntry();
                    Entry prev_entry(sel_entry);
                    pu::audio::PlaySfx(this->entry_move_sfx);
                    if(sel_entry.MoveToIndex(this->entry_menu->GetFocusedEntryIndex())) {
                        this->StopSelection();
                        this->entry_menu->NotifyEntryRemoved(prev_entry);
                        this->entry_menu->NotifyEntryAdded(sel_entry);
                        this->entry_menu->OrganizeUpdateEntries();
                    }
                    else {
                        // Should not happen...
                    }
                }
                else {
                    const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("menu_new_entry"), GetLanguageString("menu_new_entry_conf"), { GetLanguageString("menu_new_folder"), GetLanguageString("menu_add_hb"), GetLanguageString("cancel") }, true);
                    if(option == 0) {
                        SwkbdConfig cfg;
                        UL_RC_ASSERT(swkbdCreate(&cfg, 0));
                        swkbdConfigSetGuideText(&cfg, GetLanguageString("swkbd_folder_name_guide").c_str());
                        char new_folder_name[500] = {};
                        const auto rc = swkbdShow(&cfg, new_folder_name, sizeof(new_folder_name));
                        swkbdClose(&cfg);
                        if(R_SUCCEEDED(rc)) {
                            pu::audio::PlaySfx(this->create_folder_sfx);

                            const auto folder_entry = CreateFolderEntry(this->entry_menu->GetPath(), new_folder_name, this->entry_menu->GetFocusedEntryIndex());
                            this->entry_menu->NotifyEntryAdded(folder_entry);
                            this->entry_menu->OrganizeUpdateEntries();
                            g_MenuApplication->ShowNotification(GetLanguageString("menu_folder_created"));
                        }
                    }
                    else if(option == 1) {
                        UL_RC_ASSERT(smi::ChooseHomebrew());
                        g_MenuApplication->Finalize();
                    }
                }
            }
        }
        else if(keys_down & HidNpadButton_Y) {
            if(!this->entry_menu->IsAnySelected() && this->entry_menu->IsFocusedNonemptyEntry()) {
                pu::audio::PlaySfx(this->entry_select_sfx);
                this->entry_menu->ToggleFocusedEntrySelected();
            }
        }
        else if(keys_down & HidNpadButton_X) {
            if(this->entry_menu->IsAnySelected()) {
                pu::audio::PlaySfx(this->entry_cancel_select_sfx);
                
                this->StopSelection();
            }
            else if(this->entry_menu->IsFocusedNonemptyEntry()) {
                auto &cur_entry = this->entry_menu->GetFocusedEntry();

                if(g_MenuApplication->IsSuspended() && g_MenuApplication->IsEntrySuspended(cur_entry)) {
                    this->HandleCloseSuspended();
                }
                else {
                    if(cur_entry.Is<EntryType::Folder>()) {
                        std::vector<std::string> options = { GetLanguageString("entry_folder_rename"), GetLanguageString("entry_remove") };
                        if(!this->entry_menu->IsInRoot()) {
                            options.push_back(GetLanguageString("entry_move_parent"));
                            options.push_back(GetLanguageString("entry_move_root"));
                        }
                        options.push_back(GetLanguageString("cancel"));
                        const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("entry_options"), GetLanguageString("entry_action"), options, true);
                        if(option == 0) {
                            SwkbdConfig cfg;
                            UL_RC_ASSERT(swkbdCreate(&cfg, 0));
                            swkbdConfigSetInitialText(&cfg, cur_entry.folder_info.name);
                            swkbdConfigSetGuideText(&cfg, GetLanguageString("swkbd_folder_name_guide").c_str());
                            char new_folder_name[500] = {};
                            const auto rc = swkbdShow(&cfg, new_folder_name, sizeof(new_folder_name));
                            swkbdClose(&cfg);
                            
                            if(R_SUCCEEDED(rc)) {
                                util::CopyToStringBuffer(cur_entry.folder_info.name, new_folder_name);
                                cur_entry.Save();
                                this->entry_menu->OrganizeUpdateEntries();
                                g_MenuApplication->ShowNotification(GetLanguageString("menu_folder_renamed"));
                            }
                        }
                        else if(option == 1) {
                            const auto option_2 = g_MenuApplication->DisplayDialog(GetLanguageString("entry_remove"), GetLanguageString("entry_remove_conf"), { GetLanguageString("yes"), GetLanguageString("no") }, true);
                            if(option_2 == 0) {
                                this->RemoveEntry(cur_entry);
                                g_MenuApplication->ShowNotification(GetLanguageString("entry_remove_ok"));
                            }
                        }
                        else if(option == 2) {
                            this->MoveEntryToParentFolder(cur_entry);
                        }
                        else if(option == 3) {
                            this->MoveEntryToRoot(cur_entry);
                        }
                    }
                    else if(cur_entry.Is<EntryType::Homebrew>()) {
                        std::vector<std::string> options = { GetLanguageString("entry_remove") };
                        if(!this->entry_menu->IsInRoot()) {
                            options.push_back(GetLanguageString("entry_move_parent"));
                            options.push_back(GetLanguageString("entry_move_root"));
                        }
                        options.push_back(GetLanguageString("cancel"));
                        const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("entry_options"), GetLanguageString("entry_action"), options, true);
                        if(option == 0) {
                            if(IsEntryNonRemovable(cur_entry)) {
                                g_MenuApplication->ShowNotification(GetLanguageString("entry_remove_special"));
                            }
                            else {
                                const auto option_2 = g_MenuApplication->DisplayDialog(GetLanguageString("entry_remove"), GetLanguageString("entry_remove_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                                if(option_2 == 0) {
                                    this->RemoveEntry(cur_entry);
                                    g_MenuApplication->ShowNotification(GetLanguageString("entry_remove_ok"));
                                }
                            }
                        }
                        else if(option == 1) {
                            this->MoveEntryToParentFolder(cur_entry);
                        }
                        else if(option == 2) {
                            this->MoveEntryToRoot(cur_entry);
                        }
                    }
                    else if(cur_entry.Is<EntryType::Application>()) {
                        std::vector<std::string> options = { GetLanguageString("app_take_over") };
                        if(!this->entry_menu->IsInRoot()) {
                            options.push_back(GetLanguageString("entry_move_parent"));
                            options.push_back(GetLanguageString("entry_move_root"));
                        }
                        options.push_back(GetLanguageString("cancel"));
                        const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("entry_options"), GetLanguageString("entry_action"), options, true);
                        if(option == 0) {
                            const auto option_2 = g_MenuApplication->DisplayDialog(GetLanguageString("app_launch"), GetLanguageString("app_take_over_select"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                            if(option_2 == 0) {
                                g_MenuApplication->SetTakeoverApplicationId(cur_entry.app_info.record.application_id);
                                g_MenuApplication->ShowNotification(GetLanguageString("app_take_over_done"));
                            }
                        }
                        else if(option == 1) {
                            this->MoveEntryToParentFolder(cur_entry);
                        }
                        else if(option == 2) {
                            this->MoveEntryToRoot(cur_entry);
                        }
                    }
                    else if(cur_entry.IsSpecial()) {
                        std::vector<std::string> options = { };
                        if(!this->entry_menu->IsInRoot()) {
                            options.push_back(GetLanguageString("entry_move_parent"));
                            options.push_back(GetLanguageString("entry_move_root"));
                        }
                        options.push_back(GetLanguageString("cancel"));
                        const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("entry_options"), GetLanguageString("entry_action"), options, true);
                        if(option == 0) {
                            this->MoveEntryToParentFolder(cur_entry);
                        }
                        else if(option == 1) {
                            this->MoveEntryToRoot(cur_entry);
                        }
                    }
                }
            }
        }
        else if(keys_down & HidNpadButton_L) {
            pu::audio::PlaySfx(this->page_move_sfx);
            this->entry_menu->MoveToPreviousPage();
        }
        else if(keys_down & HidNpadButton_R) {
            pu::audio::PlaySfx(this->page_move_sfx);
            this->entry_menu->MoveToNextPage();
        }
    }

    void MainMenuLayout::menu_FocusedEntryChanged(const bool has_prev_entry, const bool is_prev_entry_suspended, const bool is_cur_entry_suspended) {
        this->cur_entry_main_text->SetVisible(true);
        this->cur_entry_sub_text->SetVisible(true);

        this->entry_menu_left_icon->SetVisible(!this->entry_menu->IsMenuStart());

        g_MenuApplication->UpdateMenuIndex(this->entry_menu->GetFocusedEntryIndex());

        if(this->entry_menu->IsFocusedNonemptyEntry()) {
            auto &cur_entry = this->entry_menu->GetFocusedEntry();
            if(cur_entry.Is<EntryType::Folder>()) {
                this->SetTopMenuFolder();
                // TODO: show folder entry count?
                // this->cur_entry_author_text->SetText(std::to_string(folder_entry_count) + " " + ((folder_entry_count == 1) ? "One entry" : "Multiple entries";
                this->cur_entry_main_text->SetText(cur_entry.folder_info.name);
                this->cur_entry_sub_text->SetVisible(false);
            }
            else if(cur_entry.Is<EntryType::SpecialEntryMiiEdit>()) {
                this->SetTopMenuDefault();
                this->cur_entry_main_text->SetText(GetLanguageString("special_entry_text_mii_edit"));
                this->cur_entry_sub_text->SetVisible(false);
            }
            else if(cur_entry.Is<EntryType::SpecialEntryWebBrowser>()) {
                this->SetTopMenuDefault();
                this->cur_entry_main_text->SetText(GetLanguageString("special_entry_text_web_browser"));
                this->cur_entry_sub_text->SetVisible(false);
            }
            else if(cur_entry.Is<EntryType::SpecialEntryUserPage>()) {
                this->SetTopMenuDefault();
                this->cur_entry_main_text->SetText(g_UserName);
                this->cur_entry_sub_text->SetVisible(false);
            }
            else if(cur_entry.Is<EntryType::SpecialEntrySettings>()) {
                this->SetTopMenuDefault();
                this->cur_entry_main_text->SetText(GetLanguageString("special_entry_text_settings"));
                this->cur_entry_sub_text->SetVisible(false);
            }
            else if(cur_entry.Is<EntryType::SpecialEntryThemes>()) {
                this->SetTopMenuDefault();
                this->cur_entry_main_text->SetText(GetLanguageString("special_entry_text_themes"));
                this->cur_entry_sub_text->SetVisible(false);
            }
            else if(cur_entry.Is<EntryType::SpecialEntryControllers>()) {
                this->SetTopMenuDefault();
                this->cur_entry_main_text->SetText(GetLanguageString("special_entry_text_controllers"));
                this->cur_entry_sub_text->SetVisible(false);
            }
            else if(cur_entry.Is<EntryType::SpecialEntryAlbum>()) {
                this->SetTopMenuDefault();
                this->cur_entry_main_text->SetText(GetLanguageString("special_entry_text_album"));
                this->cur_entry_sub_text->SetVisible(false);
            }
            else {
                if(cur_entry.Is<EntryType::Application>()) {
                    this->SetTopMenuApplication();
                }
                else {
                    this->SetTopMenuHomebrew();
                }
                
                cur_entry.TryLoadControlData();
                if(cur_entry.control.IsLoaded()) {
                    this->cur_entry_main_text->SetText(cur_entry.control.name);
                    this->cur_entry_sub_text->SetText(cur_entry.control.version + ", " + cur_entry.control.author);
                }
                else {
                    // TODO (low priority): anything better to show when control data doesn't load? (really shouldn't happen anyway)
                    this->cur_entry_main_text->SetText("???");
                    this->cur_entry_sub_text->SetText("???");
                }
            }
        }
        else {
            this->SetTopMenuDefault();
            this->cur_entry_main_text->SetVisible(false);
            this->cur_entry_sub_text->SetVisible(false);
        }

        if(g_MenuApplication->IsSuspended() && has_prev_entry) {
            if(is_prev_entry_suspended && !is_cur_entry_suspended) {
                this->mode = SuspendedImageMode::HidingLostFocus;
            }
            else if(!is_prev_entry_suspended && is_cur_entry_suspended) {
                this->mode = SuspendedImageMode::ShowingGainedFocus;
            }
        }
    }

    MainMenuLayout::MainMenuLayout(const u8 *captured_screen_buf, const u8 min_alpha) : IMenuLayout(), last_has_connection(false), last_connection_strength(0), last_battery_lvl(0), last_is_charging(false), last_quick_menu_on(false), start_time_elapsed(false), min_alpha(min_alpha), mode(SuspendedImageMode::ShowingAfterStart), suspended_screen_alpha(0xFF) {
        // TODO (low priority): like nxlink but for sending themes and quickly being able to test them?
        this->cur_folder_path = g_MenuApplication->GetStatus().last_menu_path;

        g_WeekdayList.clear();
        for(u32 i = 0; i < 7; i++) {
            g_WeekdayList.push_back(GetLanguageString("week_day_short_" + std::to_string(i)));
        }

        if(captured_screen_buf != nullptr) {
            this->suspended_screen_img = RawRgbaImage::New(0, 0, captured_screen_buf, 1280, 720, 4);
            this->suspended_screen_img->SetWidth(pu::ui::render::ScreenWidth);
            this->suspended_screen_img->SetHeight(pu::ui::render::ScreenHeight);
        }
        else {
            this->suspended_screen_img = nullptr;
        }

        this->quick_menu = nullptr;

        // Load banners first
        this->top_menu_default_bg = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/TopMenuDefaultBackground"));
        this->top_menu_folder_bg = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/TopMenuFolderBackground"));
        this->top_menu_app_bg = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/TopMenuApplicationBackground"));
        this->top_menu_hb_bg = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/TopMenuHomebrewBackground"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "top_menu_bg", this->top_menu_default_bg);
        g_MenuApplication->ApplyConfigForElement("main_menu", "top_menu_bg", this->top_menu_folder_bg);
        g_MenuApplication->ApplyConfigForElement("main_menu", "top_menu_bg", this->top_menu_app_bg);
        g_MenuApplication->ApplyConfigForElement("main_menu", "top_menu_bg", this->top_menu_hb_bg);
        this->Add(this->top_menu_default_bg);
        this->Add(this->top_menu_folder_bg);
        this->Add(this->top_menu_app_bg);
        this->Add(this->top_menu_hb_bg);

        // Then load buttons and other UI elements
        this->logo_top_icon = ClickableImage::New(0, 0, GetLogoTexture());
        this->logo_top_icon->SetWidth(LogoSize);
        this->logo_top_icon->SetHeight(LogoSize);
        this->logo_top_icon->SetOnClick(&ShowAboutDialog);
        g_MenuApplication->ApplyConfigForElement("main_menu", "logo_top_icon", this->logo_top_icon, false); // Sorry theme makers... uLaunch's logo must be visible, but can be moved
        this->Add(this->logo_top_icon);

        this->connection_top_icon = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/ConnectionTopIcon/None"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "connection_top_icon", this->connection_top_icon);
        this->Add(this->connection_top_icon);

        this->time_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        this->time_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "time_text", this->time_text);
        this->Add(this->time_text);

        this->date_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        this->date_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "date_text", this->date_text);
        this->Add(this->date_text);

        this->battery_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        this->battery_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "battery_text", this->battery_text);
        this->Add(this->battery_text);

        this->battery_top_icon = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/BatteryTopIcon/100"));
        this->battery_charging_top_icon = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/BatteryTopIcon/Charging"));
        this->battery_charging_top_icon->SetVisible(false);
        g_MenuApplication->ApplyConfigForElement("main_menu", "battery_top_icon", this->battery_top_icon);
        g_MenuApplication->ApplyConfigForElement("main_menu", "battery_top_icon", this->battery_charging_top_icon);
        this->Add(this->battery_top_icon);
        this->Add(this->battery_charging_top_icon);

        this->input_bar = InputBar::New(0, 0);
        g_MenuApplication->ApplyConfigForElement("main_menu", "input_bar", this->input_bar);
        this->Add(this->input_bar);

        this->cur_path_text = pu::ui::elm::TextBlock::New(0, 0, this->cur_folder_path);
        this->cur_path_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "cur_path_text", this->cur_path_text);

        this->cur_entry_main_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        this->cur_entry_main_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "cur_entry_main_text", this->cur_entry_main_text);

        this->cur_entry_sub_text = pu::ui::elm::TextBlock::New(0, 0, "...");
        this->cur_entry_sub_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "cur_entry_sub_text", this->cur_entry_sub_text);

        this->entry_menu_bg = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/EntryMenuBackground"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "entry_menu_bg", this->entry_menu_bg);
        this->Add(this->entry_menu_bg);

        this->entry_menu_left_icon = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/EntryMenuLeftIcon"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "entry_menu_left_icon", this->entry_menu_left_icon);
        this->Add(this->entry_menu_left_icon);

        this->entry_menu_right_icon = pu::ui::elm::Image::New(0, 0, TryFindLoadImageHandle("ui/EntryMenuRightIcon"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "entry_menu_right_icon", this->entry_menu_right_icon);
        this->Add(this->entry_menu_right_icon);

        this->Add(this->cur_entry_main_text);
        this->Add(this->cur_entry_sub_text);

        this->Add(this->cur_path_text);

        this->entry_menu = EntryMenu::New(0, 0, g_MenuApplication->GetStatus().last_menu_fs_path, std::bind(&MainMenuLayout::menu_EntryInputPressed, this, std::placeholders::_1), std::bind(&MainMenuLayout::menu_FocusedEntryChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), [&]() {
            pu::audio::PlaySfx(this->cursor_move_sfx);
        });
        g_MenuApplication->ApplyConfigForElement("main_menu", "entry_menu", this->entry_menu);
        this->Add(this->entry_menu);

        if(captured_screen_buf != nullptr) {
            this->Add(this->suspended_screen_img);
        }

        this->quick_menu = QuickMenu::New();
        this->Add(this->quick_menu);

        this->startup_tp = std::chrono::steady_clock::now();

        this->post_suspend_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/PostSuspend.wav"));
        this->cursor_move_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/CursorMove.wav"));
        this->page_move_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/PageMove.wav"));
        this->entry_select_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/EntrySelect.wav"));
        this->entry_move_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/EntryMove.wav"));
        this->entry_swap_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/EntrySwap.wav"));
        this->entry_cancel_select_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/EntryCancelSelect.wav"));
        this->entry_move_into_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/EntryMoveInto.wav"));
        this->home_press_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/HomePress.wav"));
        this->logoff_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/Logoff.wav"));
        this->launch_app_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/LaunchApplication.wav"));
        this->launch_hb_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/LaunchHomebrew.wav"));
        this->close_suspended_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/CloseSuspended.wav"));
        this->open_folder_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/OpenFolder.wav"));
        this->close_folder_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/CloseFolder.wav"));
        this->open_mii_edit_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/OpenMiiEdit.wav"));
        this->open_web_browser_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/OpenWebBrowser.wav"));
        this->open_user_page_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/OpenUserPage.wav"));
        this->open_settings_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/OpenSettings.wav"));
        this->open_themes_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/OpenThemes.wav"));
        this->open_controllers_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/OpenControllers.wav"));
        this->open_album_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/OpenAlbum.wav"));
        this->open_quick_menu_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/OpenQuickMenu.wav"));
        this->close_quick_menu_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/CloseQuickMenu.wav"));
        this->resume_app_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/ResumeApplication.wav"));
        this->create_folder_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/CreateFolder.wav"));
        this->create_hb_entry_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/CreateHomebrewEntry.wav"));
        this->entry_remove_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/EntryRemove.wav"));
        this->error_sfx = pu::audio::LoadSfx(TryGetActiveThemeResource("sound/Main/Error.wav"));

        if(captured_screen_buf != nullptr) {
            pu::audio::PlaySfx(this->post_suspend_sfx);
        }
    }

    MainMenuLayout::~MainMenuLayout() {
        pu::audio::DestroySfx(this->post_suspend_sfx);
        pu::audio::DestroySfx(this->cursor_move_sfx);
        pu::audio::DestroySfx(this->page_move_sfx);
        pu::audio::DestroySfx(this->entry_select_sfx);
        pu::audio::DestroySfx(this->entry_move_sfx);
        pu::audio::DestroySfx(this->entry_swap_sfx);
        pu::audio::DestroySfx(this->entry_cancel_select_sfx);
        pu::audio::DestroySfx(this->entry_move_into_sfx);
        pu::audio::DestroySfx(this->home_press_sfx);
        pu::audio::DestroySfx(this->logoff_sfx);
        pu::audio::DestroySfx(this->launch_app_sfx);
        pu::audio::DestroySfx(this->launch_hb_sfx);
        pu::audio::DestroySfx(this->close_suspended_sfx);
        pu::audio::DestroySfx(this->open_folder_sfx);
        pu::audio::DestroySfx(this->close_folder_sfx);
        pu::audio::DestroySfx(this->open_mii_edit_sfx);
        pu::audio::DestroySfx(this->open_web_browser_sfx);
        pu::audio::DestroySfx(this->open_user_page_sfx);
        pu::audio::DestroySfx(this->open_settings_sfx);
        pu::audio::DestroySfx(this->open_themes_sfx);
        pu::audio::DestroySfx(this->open_controllers_sfx);
        pu::audio::DestroySfx(this->open_album_sfx);
        pu::audio::DestroySfx(this->open_quick_menu_sfx);
        pu::audio::DestroySfx(this->close_quick_menu_sfx);
        pu::audio::DestroySfx(this->resume_app_sfx);
        pu::audio::DestroySfx(this->create_folder_sfx);
        pu::audio::DestroySfx(this->create_hb_entry_sfx);
        pu::audio::DestroySfx(this->entry_remove_sfx);
        pu::audio::DestroySfx(this->error_sfx);
    }

    void MainMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        const auto quick_menu_on = this->quick_menu->IsOn();
        if(this->last_quick_menu_on != quick_menu_on) {
            this->last_quick_menu_on = quick_menu_on;
            this->entry_menu->SetEnabled(!quick_menu_on);

            if(quick_menu_on) {
                pu::audio::PlaySfx(this->open_quick_menu_sfx);
            }
            else {
                pu::audio::PlaySfx(this->close_quick_menu_sfx);
            }
        }
        if(quick_menu_on) {
            return;
        }

        ////////////////////////////////////////////////////////

        this->input_bar->ClearInputs();
        if(this->entry_menu->IsFocusedNonemptyEntry()) {
            if(this->entry_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_move_selected"));
            }
            else if(this->entry_menu->IsFocusedEntrySuspended()) {
                this->input_bar->AddSetInput(HidNpadButton_A | InputBar::MetaHomeNpadButton, GetLanguageString("input_resume_suspended"));
            }
            else {
                const auto &cur_entry = this->entry_menu->GetFocusedEntry();
                if(cur_entry.Is<EntryType::Folder>()) {
                    this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_open_folder"));
                }
                else {
                    this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_launch_entry"));
                }
            }

            if(this->entry_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_X, GetLanguageString("input_cancel_selection"));
            }
            else if(this->entry_menu->IsFocusedEntrySuspended()) {
                this->input_bar->AddSetInput(HidNpadButton_X, GetLanguageString("input_close_suspended"));
            }
            else if(this->entry_menu->IsFocusedNonemptyEntry()) {
                const auto &cur_entry = this->entry_menu->GetFocusedEntry();
                if(!cur_entry.IsSpecial()) {
                    this->input_bar->AddSetInput(HidNpadButton_X, GetLanguageString("input_entry_options"));
                }
            }

            if(!this->entry_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_Y, GetLanguageString("input_select_entry"));
            }

            if(this->entry_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_B, GetLanguageString("input_cancel_selection"));
            }
            else if(!this->entry_menu->IsInRoot()) {
                this->input_bar->AddSetInput(HidNpadButton_B, GetLanguageString("input_folder_back"));
            }
        }
        else {
            if(this->entry_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_move_selected"));
                this->input_bar->AddSetInput(HidNpadButton_B, GetLanguageString("input_cancel_selection"));
                this->input_bar->AddSetInput(HidNpadButton_X, GetLanguageString("input_cancel_selection"));
            }
            else {
                this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_new_entry"));
            }
        }

        if(this->entry_menu->IsMenuStart()) {
            this->input_bar->AddSetInput(InputBar::MetaDpadNpadButton | InputBar::MetaAnyStickNpadButton | HidNpadButton_R, GetLanguageString("input_navigate"));
        }
        else {
            this->input_bar->AddSetInput(InputBar::MetaDpadNpadButton | InputBar::MetaAnyStickNpadButton | HidNpadButton_L | HidNpadButton_R, GetLanguageString("input_navigate"));
        }

        if(this->entry_menu->IsInRoot() && !this->entry_menu->IsAnySelected()) {
            this->input_bar->AddSetInput(HidNpadButton_B, GetLanguageString("input_logoff"));
        }

        if(g_MenuApplication->IsSuspended() && !this->entry_menu->IsFocusedEntrySuspended()) {
            this->input_bar->AddSetInput(InputBar::MetaHomeNpadButton, GetLanguageString("input_resume_suspended"));
        }

        this->input_bar->AddSetInput(HidNpadButton_Plus | HidNpadButton_Minus, GetLanguageString("input_resize_menu"));

        this->input_bar->AddSetInput(HidNpadButton_ZL | HidNpadButton_ZR, GetLanguageString("input_quick_menu"));

        ///////////////////////////////

        const auto now_tp = std::chrono::steady_clock::now();

        u32 conn_strength;
        const auto has_conn = net::HasConnection(conn_strength);
        if((this->last_has_connection != has_conn) || (this->last_connection_strength != conn_strength)) {
            this->last_has_connection = has_conn;
            this->last_connection_strength = conn_strength;
            if(has_conn) {
                this->connection_top_icon->SetImage(TryFindLoadImageHandle("ui/ConnectionTopIcon/" + std::to_string(conn_strength)));
            }
            else {
                this->connection_top_icon->SetImage(TryFindLoadImageHandle("ui/ConnectionTopIcon/None"));
            }
        }

        u32 cur_h;
        u32 cur_min;
        u32 cur_sec;
        os::GetCurrentTime(cur_h, cur_min, cur_sec);
        char cur_time_str[0x10] = {};
        sprintf(cur_time_str, "%02d:%02d", cur_h, cur_min);
        this->time_text->SetText(cur_time_str);

        // TODO (low priority): do not set the date all the time?
        const auto cur_date = os::GetCurrentDate(g_WeekdayList);
        this->date_text->SetText(cur_date);

        const auto battery_lvl = os::GetBatteryLevel();
        const auto is_charging = os::IsConsoleCharging();
        if((this->last_battery_lvl != battery_lvl) || (this->last_is_charging != is_charging)) {
            this->last_battery_lvl = battery_lvl;
            this->last_is_charging = is_charging;

            const auto battery_str = std::to_string(battery_lvl) + "%";
            this->battery_text->SetText(battery_str);

            auto battery_lvl_norm = (1 + (battery_lvl / 10)) * 10; // Converts it to 10, 20, ..., 100
            if(battery_lvl_norm > 100) {
                battery_lvl_norm = 100;
            }
            const auto battery_img = "ui/BatteryTopIcon/" + std::to_string(battery_lvl_norm);
            this->battery_top_icon->SetImage(TryFindLoadImageHandle(battery_img));
            this->battery_charging_top_icon->SetVisible(is_charging);
        }

        if(!this->start_time_elapsed) {
            // Wait a bit before handling sent messages
            if(std::chrono::duration_cast<std::chrono::seconds>(now_tp - this->startup_tp).count() >= MessagesWaitTimeSeconds) {
                this->start_time_elapsed = true;
            }
        }

        if(this->start_time_elapsed) {
            if(g_MenuApplication->GetConsumeLastLaunchFailed()) {
                pu::audio::PlaySfx(this->error_sfx);
                g_MenuApplication->DisplayDialog(GetLanguageString("app_launch"), GetLanguageString("app_unexpected_error"), { GetLanguageString("ok") }, true);
            }
            else if(g_MenuApplication->HasChosenHomebrew()) {
                const auto nro_path = g_MenuApplication->GetConsumeChosenHomebrew();
                pu::audio::PlaySfx(this->create_hb_entry_sfx);

                // TODO (low priority): custom argv option?
                const auto hb_entry = CreateHomebrewEntry(this->entry_menu->GetPath(), nro_path, nro_path, this->entry_menu->GetFocusedEntryIndex());
                this->entry_menu->NotifyEntryAdded(hb_entry);
                this->entry_menu->OrganizeUpdateEntries();
                g_MenuApplication->ShowNotification(GetLanguageString("menu_chosen_hb_added"));
            }
            else if(g_MenuApplication->HasGameCardMountFailure()) {
                const auto gc_rc = g_MenuApplication->GetConsumeGameCardMountFailure();
                pu::audio::PlaySfx(this->error_sfx);

                g_MenuApplication->DisplayDialog(GetLanguageString("gamecard"), GetLanguageString("gamecard_mount_failed") + " " + util::FormatResultDisplay(gc_rc), { GetLanguageString("ok") }, true);
            }
        }

        if(this->suspended_screen_img) {
            switch(this->mode) {
                case SuspendedImageMode::ShowingAfterStart: {
                    if(this->entry_menu->IsFocusedEntrySuspended() && (this->suspended_screen_alpha <= this->min_alpha)) {
                        this->suspended_screen_alpha = this->min_alpha;
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->mode = SuspendedImageMode::Focused;
                    }
                    else if(!this->entry_menu->IsFocusedEntrySuspended() && (this->suspended_screen_alpha == 0)) {
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->mode = SuspendedImageMode::NotFocused;
                    }
                    else {
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->suspended_screen_alpha -= SuspendedScreenAlphaIncrement;
                        if(this->suspended_screen_alpha < 0) {
                            this->suspended_screen_alpha = 0;
                        }
                    }
                    break;
                }
                case SuspendedImageMode::Focused: {
                    break;
                }
                case SuspendedImageMode::HidingForResume: {
                    if(this->suspended_screen_alpha == 0xFF) {
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        UL_RC_ASSERT(smi::ResumeApplication());
                    }
                    else {
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->suspended_screen_alpha += SuspendedScreenAlphaIncrement;
                        if(this->suspended_screen_alpha > 0xFF) {
                            this->suspended_screen_alpha = 0xFF;
                        }
                    }
                    break;
                }
                case SuspendedImageMode::NotFocused: {
                    break;
                }
                case SuspendedImageMode::ShowingGainedFocus: {
                    if(this->suspended_screen_alpha == this->min_alpha) {
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->mode = SuspendedImageMode::Focused;
                    }
                    else {
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->suspended_screen_alpha += SuspendedScreenAlphaIncrement;
                        if(this->suspended_screen_alpha > this->min_alpha) {
                            this->suspended_screen_alpha = this->min_alpha;
                        }
                    }
                    break;
                }
                case SuspendedImageMode::HidingLostFocus: {
                    if(this->suspended_screen_alpha == 0) {
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->mode = SuspendedImageMode::NotFocused;
                    }
                    else {
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->suspended_screen_alpha -= SuspendedScreenAlphaIncrement;
                        if(this->suspended_screen_alpha < 0) {
                            this->suspended_screen_alpha = 0;
                        }
                    }
                    break;
                }
            }
        }

        if(keys_down & HidNpadButton_Minus) {
            this->entry_menu->DecrementEntryHeightCount();
        }
        else if(keys_down & HidNpadButton_Plus) {
            this->entry_menu->IncrementEntryHeightCount();
        }
    }

    bool MainMenuLayout::OnHomeButtonPress() {
        pu::audio::PlaySfx(this->home_press_sfx);

        if(g_MenuApplication->IsSuspended()) {
            this->StartResume();
        }
        else {
            if(!this->entry_menu->IsInRoot()) {
                this->MoveToRoot(true);
            }

            this->entry_menu->Rewind();
        }

        return true;
    }

    void MainMenuLayout::MoveTo(const std::string &new_path, const bool fade, std::function<void()> action) {
        if(fade) {
            g_MenuApplication->SetBackgroundFade();
            g_MenuApplication->FadeOut();

            if(action) {
                action();
            }
            this->DoMoveTo(new_path);

            g_MenuApplication->FadeIn();
        }
        else {
            this->DoMoveTo(new_path);
        }
    }

    void MainMenuLayout::NotifyLoad(const AccountUid user) {
        UL_RC_ASSERT(acc::GetAccountName(user, g_UserName));
        this->entry_menu->Initialize(g_MenuApplication->GetStatus().last_menu_index);
        this->quick_menu->UpdateItems();
    }

    void MainMenuLayout::HandleCloseSuspended() {
        const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("suspended_app"), GetLanguageString("suspended_app_close"), { GetLanguageString("yes"), GetLanguageString("no") }, true);
        if(option == 0) {
            this->DoTerminateApplication();
        }
    }

    void MainMenuLayout::HandleHomebrewLaunch(const Entry &hb_entry) {
        const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("hb_launch"), GetLanguageString("hb_launch_conf"), { GetLanguageString("hb_applet"), GetLanguageString("hb_app"), GetLanguageString("cancel") }, true);
        if(option == 0) {
            pu::audio::PlaySfx(this->launch_hb_sfx);
            
            const auto proper_ipt = CreateLaunchTargetInput(hb_entry.hb_info.nro_target);
            UL_RC_ASSERT(smi::LaunchHomebrewLibraryApplet(proper_ipt.nro_path, proper_ipt.nro_argv));

            g_MenuApplication->Finalize();
        }
        else if(option == 1) {
            if(g_MenuApplication->GetTakeoverApplicationId() != 0) {
                auto launch = true;
                if(g_MenuApplication->IsSuspended()) {
                    launch = false;
                    this->HandleCloseSuspended();
                    if(!g_MenuApplication->IsSuspended()) {
                        launch = true;
                    }
                }
                if(launch) {
                    pu::audio::PlaySfx(this->launch_hb_sfx);
                    
                    const auto ipt = CreateLaunchTargetInput(hb_entry.hb_info.nro_target);
                    const auto rc = smi::LaunchHomebrewApplication(ipt.nro_path, ipt.nro_argv);

                    if(R_SUCCEEDED(rc)) {
                        g_MenuApplication->Finalize();
                        return;
                    }
                    else {
                        g_MenuApplication->ShowNotification(GetLanguageString("app_launch_error") + ": " + util::FormatResultDisplay(rc));
                    }
                }
            }
            else {
                g_MenuApplication->DisplayDialog(GetLanguageString("app_launch"), GetLanguageString("app_no_take_over_app"), { GetLanguageString("ok") }, true);
            }
        }
    }

    void MainMenuLayout::StopSelection() {
        this->entry_menu->ResetSelection();
    }

    void MainMenuLayout::DoTerminateApplication() {
        pu::audio::PlaySfx(this->close_suspended_sfx);

        auto &entries = this->entry_menu->GetEntries();
        u32 i = 0;
        for(const auto &entry : entries) {
            if(g_MenuApplication->IsEntrySuspended(entry)) {
                break;
            }
            i++;
        }

        UL_RC_ASSERT(smi::TerminateApplication());

        if(i < entries.size()) {
            // We need to reload the application record since its kind/type changed after closing it
            // (only if the entry is loaded, aka in the current folder)
            entries.at(i).ReloadApplicationInfo();
        }

        g_MenuApplication->ResetSuspendedApplication();

        this->mode = SuspendedImageMode::NotFocused;
        if(this->suspended_screen_img) {
            this->suspended_screen_img->SetAlpha(0);
        }
    }

}
