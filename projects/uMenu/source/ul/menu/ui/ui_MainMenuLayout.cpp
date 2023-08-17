#include <ul/menu/ui/ui_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_Stl.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/util/util_String.hpp>
#include <ul/net/net_Service.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/os/os_System.hpp>
#include <ul/os/os_Applications.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::menu::ui::TransitionGuard g_TransitionGuard;
extern ul::cfg::Config g_Config;
extern ul::cfg::Theme g_Theme;

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

        bool g_CurrentThemeChecked = false;

    }

    void MainMenuLayout::DoMoveTo(const std::string &new_path) {
        // Empty path used as a "reload" argumnt
        if(!new_path.empty()) {
            char menu_path[FS_MAX_PATH];
            util::CopyToStringBuffer(menu_path, new_path);
            UL_RC_ASSERT(smi::UpdateMenuPath(menu_path));
        }

        this->entries_menu->MoveTo(new_path);

        if(this->entries_menu->HasEntries()) {
            this->banner_img->SetVisible(true);
            this->cur_entry_name_text->SetVisible(true);
            this->cur_entry_author_text->SetVisible(true);
            this->cur_entry_version_text->SetVisible(true);
            this->no_entries_text->SetVisible(false);
        }
        else {
            this->banner_img->SetVisible(false);
            this->cur_entry_name_text->SetVisible(false);
            this->cur_entry_author_text->SetVisible(false);
            this->cur_entry_version_text->SetVisible(false);
            this->no_entries_text->SetVisible(true);
        }
    }

    void MainMenuLayout::menu_EntryInputPressed(const u64 keys_down) {
        if(!this->entries_menu->HasEntries()) {
            return;
        }

        auto &cur_entry = this->entries_menu->GetFocusedEntry();

        if(keys_down & HidNpadButton_B) {
            if(this->entries_menu->IsAnySelected()) {
                const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_selection"), GetLanguageString("menu_select_cancel_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                if(option == 0) {
                    g_MenuApplication->ShowNotification(GetLanguageString("menu_select_cancel"));
                    this->StopSelection();
                }
            }
            else if(!this->entries_menu->IsInRoot()) {
                const auto parent_path = fs::GetBaseDirectory(this->entries_menu->GetPath());
                this->MoveTo(parent_path, true);
            }
        }
        else if(keys_down & HidNpadButton_A) {
            if(this->entries_menu->IsAnySelected()) {
                if(cur_entry.Is<EntryType::Folder>()) {
                    if(this->entries_menu->IsFocusedEntrySelected()) {
                        g_MenuApplication->ShowNotification(GetLanguageString("menu_move_folder_itself"));
                    }
                    else {
                        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_selection"), GetLanguageString("menu_move_to_folder_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                        if(option == 0) {
                            u32 cur_i = 0;
                            for(auto &entry : this->entries_menu->GetEntries()) {
                                if(this->entries_menu->IsEntrySelected(cur_i)) {
                                    entry.MoveTo(cur_entry.GetFolderPath());
                                }

                                cur_i++;
                            }

                            this->StopSelection();
                            this->MoveTo("", true);
                            g_MenuApplication->ShowNotification(GetLanguageString("menu_move_ok"));
                        }
                    }
                }
                else {
                    auto &cur_entries = this->entries_menu->GetEntries();
                    const auto cur_i = this->entries_menu->GetFocusedEntryIndex();
                    const auto last_i = cur_entries.size() - 1;

                    // TODONEW: apply this to folders too? maybe pick better key inputs for this
                    const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_selection"), GetLanguageString("menu_move_around_entry_conf"), { GetLanguageString("menu_move_around_entry_before"), GetLanguageString("menu_move_around_entry_after"), GetLanguageString("menu_move_around_entry_swap"), GetLanguageString("cancel") }, true);
                    if(option == 0) {
                        auto cur_start_idx = (cur_i == 0) ? InvalidEntryIndex : cur_entries.at(cur_i - 1).index;
                        const auto cur_end_idx = cur_entry.index;
                        u32 cur_i = 0;
                        for(auto &entry : cur_entries) {
                            if(this->entries_menu->IsEntrySelected(cur_i)) {
                                // Move to the start range, then limit the range so next entries will follow this one
                                entry.OrderBetween(cur_start_idx, cur_end_idx);
                                cur_start_idx = entry.index;
                            }

                            cur_i++;
                        }

                        this->StopSelection();
                        this->MoveTo("", true);
                        g_MenuApplication->ShowNotification(GetLanguageString("menu_move_ok"));
                    }
                    else if(option == 1) {
                        auto cur_start_idx = cur_entry.index;
                        const auto cur_end_idx = (cur_i == last_i) ? InvalidEntryIndex : cur_entries.at(cur_i + 1).index;
                        u32 cur_i = 0;
                        for(auto &entry : cur_entries) {
                            if(this->entries_menu->IsEntrySelected(cur_i)) {
                                // Move to the start range, then limit the range so next entries will follow this one
                                entry.OrderBetween(cur_start_idx, cur_end_idx);
                                cur_start_idx = entry.index;
                            }

                            cur_i++;
                        }

                        this->StopSelection();
                        this->MoveTo("", true);
                        g_MenuApplication->ShowNotification(GetLanguageString("menu_move_ok"));
                    }
                    else if(option == 2) {
                        // Basically a "move before" followed by moving the current item in the range where the first selected item was
                        auto cur_start_idx = (cur_i == 0) ? InvalidEntryIndex : cur_entries.at(cur_i - 1).index;
                        const auto cur_end_idx = cur_entry.index;
                        u32 cur_i = 0;
                        u32 move_start_idx = UINT32_MAX;
                        u32 move_end_idx = UINT32_MAX;
                        for(auto &entry : cur_entries) {
                            if(this->entries_menu->IsEntrySelected(cur_i)) {
                                // This way, get the start+end index which map the range of the first selected item
                                if(move_start_idx == UINT32_MAX) {
                                    move_start_idx = entry.index;
                                }
                                else if(move_end_idx == UINT32_MAX) {
                                    move_end_idx = entry.index;
                                }
                                

                                entry.OrderBetween(cur_start_idx, cur_end_idx);
                                cur_start_idx = entry.index;
                            }

                            cur_i++;
                        }

                        cur_entry.OrderBetween(move_start_idx, move_end_idx);

                        this->StopSelection();
                        this->MoveTo("", true);
                        g_MenuApplication->ShowNotification(GetLanguageString("menu_move_ok"));
                    }
                }
            }
            else {
                if(cur_entry.Is<EntryType::Folder>()) {
                    this->PushFolder(cur_entry.folder_info.name);
                    this->MoveTo(cur_entry.GetFolderPath(), true);
                }
                else {
                    auto do_launch_entry = true;

                    if(g_MenuApplication->IsSuspended()) {
                        const auto cur_hb_suspended = cur_entry.Is<EntryType::Homebrew>() && g_MenuApplication->EqualsSuspendedHomebrewPath(cur_entry.hb_info.nro_target.nro_path);
                        const auto cur_app_suspended = cur_entry.Is<EntryType::Application>() && (cur_entry.app_info.record.application_id == g_MenuApplication->GetStatus().suspended_app_id);

                        // Play animations, then resume the suspended hb/app
                        if(cur_hb_suspended) {
                            if(this->mode == SuspendedImageMode::Focused) {
                                this->mode = SuspendedImageMode::HidingForResume;
                            }
                            do_launch_entry = false;
                        }
                        else if(cur_app_suspended) {
                            if(this->mode == SuspendedImageMode::Focused) {
                                this->mode = SuspendedImageMode::HidingForResume;
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

                    if(cur_entry.Is<EntryType::Application>() && !cur_entry.app_info.IsLaunchable()) {
                        // TODONEW: gamecard detection?
                        // TODONEW: support for "fixing" corrupted apps, like regular homemenu?
                        g_MenuApplication->ShowNotification(GetLanguageString("app_not_launchable"));
                        do_launch_entry = false;
                    }

                    if(do_launch_entry) {
                        if(cur_entry.Is<EntryType::Homebrew>()) {
                            this->HandleHomebrewLaunch(cur_entry);
                        }
                        else {
                            pu::audio::PlaySfx(this->title_launch_sfx);

                            const auto rc = smi::LaunchApplication(cur_entry.app_info.record.application_id);
                            if(R_SUCCEEDED(rc)) {
                                g_MenuApplication->StopPlayBGM();
                                g_MenuApplication->CloseWithFadeOut();
                                return;
                            }
                            else {
                                g_MenuApplication->ShowNotification(GetLanguageString("app_launch_error") + ": " + util::FormatResultDisplay(rc));
                            }
                        }
                    }
                }
            }
        }
        else if(keys_down & HidNpadButton_Y) {
            this->entries_menu->ToggleFocusedEntrySelected();
        }
        else if(keys_down & HidNpadButton_X) {
            if(this->entries_menu->IsAnySelected()) {
                const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_selection"), GetLanguageString("menu_select_cancel_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                if(option == 0) {
                    // TODONEW: above
                    g_MenuApplication->ShowNotification(GetLanguageString("menu_select_cancel"));
                    this->StopSelection();
                }
            }
            else {
                if(cur_entry.Is<EntryType::Folder>()) {
                    const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("entry_options"), GetLanguageString("entry_action"), { "Rename", GetLanguageString("entry_remove"), GetLanguageString("cancel") }, true);
                    if(option == 0) {
                        SwkbdConfig cfg;
                        // TODONEW: check results here
                        swkbdCreate(&cfg, 0);
                        swkbdConfigSetGuideText(&cfg, GetLanguageString("swkbd_rename_folder_guide").c_str());
                        char new_folder_name[500] = {};
                        const auto rc = swkbdShow(&cfg, new_folder_name, sizeof(new_folder_name));
                        swkbdClose(&cfg);
                        // TODONEW: add confirmation?
                        if(R_SUCCEEDED(rc)) {
                            util::CopyToStringBuffer(cur_entry.folder_info.name, new_folder_name);
                            cur_entry.Save();
                            this->MoveTo("", true);
                        }
                    }
                    else if(option == 1) {
                        const auto option_2 = g_MenuApplication->CreateShowDialog(GetLanguageString("entry_remove"), GetLanguageString("entry_remove_conf"), { GetLanguageString("yes"), GetLanguageString("no") }, true);
                        if(option_2 == 0) {
                            cur_entry.Remove();
                            this->MoveTo("", true);
                            g_MenuApplication->ShowNotification(GetLanguageString("entry_remove_ok"));
                        }
                    }
                }
                else if(cur_entry.Is<EntryType::Homebrew>()) {
                    const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("entry_options"), GetLanguageString("entry_action"), { GetLanguageString("entry_remove"), GetLanguageString("cancel") }, true);
                    if(option == 0) {
                        if((strcmp(cur_entry.hb_info.nro_target.nro_path, ul::HbmenuPath) == 0) || (strcmp(cur_entry.hb_info.nro_target.nro_path, ul::ManagerPath) == 0)) {
                            g_MenuApplication->ShowNotification(GetLanguageString("entry_remove_special"));
                        }
                        else {
                            const auto option_2 = g_MenuApplication->CreateShowDialog(GetLanguageString("entry_remove"), GetLanguageString("entry_remove_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                            if(option_2 == 0) {
                                cur_entry.Remove();
                                this->MoveTo("", true);
                                g_MenuApplication->ShowNotification(GetLanguageString("entry_remove_ok"));
                            }
                        }
                    }
                }
                else {
                    const auto cur_hb_suspended = cur_entry.Is<EntryType::Homebrew>() && g_MenuApplication->EqualsSuspendedHomebrewPath(cur_entry.hb_info.nro_target.nro_path);
                    const auto cur_app_suspended = cur_entry.Is<EntryType::Application>() && (cur_entry.app_info.record.application_id == g_MenuApplication->GetStatus().suspended_app_id);

                    if(g_MenuApplication->IsSuspended() && (cur_hb_suspended || cur_app_suspended)) {
                        this->HandleCloseSuspended();
                    }
                }
            }
        }
        else if(keys_down & HidNpadButton_StickL) { 
            // pu::audio::PlaySfx(this->menu_toggle_sfx);

            const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_new_entry_options"), GetLanguageString("menu_new_entry"), { GetLanguageString("menu_new_folder"), GetLanguageString("menu_add_hb"), GetLanguageString("cancel") }, true);
            if(option == 0) {
                SwkbdConfig cfg;
                // TODONEW: check results here
                swkbdCreate(&cfg, 0);
                swkbdConfigSetGuideText(&cfg, GetLanguageString("swkbd_rename_folder_guide").c_str());
                char new_folder_name[500] = {};
                const auto rc = swkbdShow(&cfg, new_folder_name, sizeof(new_folder_name));
                swkbdClose(&cfg);
                if(R_SUCCEEDED(rc)) {
                    CreateFolderEntry(this->entries_menu->GetPath(), new_folder_name);
                    this->MoveTo("", true);
                    g_MenuApplication->ShowNotification(GetLanguageString("menu_folder_created"));
                }
            }
            else if(option == 1) {
                UL_RC_ASSERT(smi::ChooseHomebrew());
                g_MenuApplication->CloseWithFadeOut();
            }
        }
        else if(keys_down & HidNpadButton_StickR) { 
            if(cur_entry.Is<EntryType::Application>()) {
                const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("app_launch"), GetLanguageString("app_take_over_select") + "\n" + GetLanguageString("app_take_over_selected"), { "Yes", "Cancel" }, true);
                if(option == 0) {
                    UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, cur_entry.app_info.record.application_id));
                    cfg::SaveConfig(g_Config);
                    g_MenuApplication->ShowNotification(GetLanguageString("app_take_over_done"));
                }
            }
        }
    }

    void MainMenuLayout::menu_FocusedEntryChanged(const bool has_prev_entry, const bool is_prev_entry_suspended, const bool is_cur_entry_suspended) {
        if(!this->entries_menu->HasEntries()) {
            return;
        }

        this->cur_entry_author_text->SetVisible(true);
        this->cur_entry_version_text->SetVisible(true);

        UL_RC_ASSERT(smi::UpdateMenuIndex(this->entries_menu->GetFocusedEntryIndex()));

        auto &cur_entry = this->entries_menu->GetFocusedEntry();
        if(cur_entry.Is<EntryType::Folder>()) {
            this->banner_img->SetImage(TryFindImage(g_Theme, "ui/BannerFolder"));

            // TODONEW: entry count?
            // this->cur_entry_author_text->SetText(std::to_string(folder_entry_count) + " " + ((folder_entry_count == 1) ? GetLanguageString("folder_entry_single") : GetLanguageString("folder_entry_mult")));
            this->cur_entry_name_text->SetText(cur_entry.folder_info.name);
            this->cur_entry_author_text->SetText(cur_entry.folder_info.fs_name);
            this->cur_entry_version_text->SetVisible(false);
        }
        else {
            if(cur_entry.Is<EntryType::Application>()) {
                this->banner_img->SetImage(TryFindImage(g_Theme, "ui/BannerInstalled"));
            }
            else {
                this->banner_img->SetImage(TryFindImage(g_Theme, "ui/BannerHomebrew"));
            }
            
            cur_entry.TryLoadControlData();
            if(cur_entry.control.IsLoaded()) {
                this->cur_entry_name_text->SetText(cur_entry.control.name);
                this->cur_entry_author_text->SetText(cur_entry.control.author);
                this->cur_entry_version_text->SetText(cur_entry.control.version);
            }
            else {
                // TODONEW: anything better to show when controldata doesnt load? (really shouldnt happen anyway)
                this->cur_entry_name_text->SetText("Unknown name...");
                this->cur_entry_author_text->SetText("Unknown author...");
                this->cur_entry_version_text->SetText("Unknown version...");
            }
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

    MainMenuLayout::MainMenuLayout(const u8 *captured_screen_buf, const u8 min_alpha) : last_has_connection(false), last_battery_lvl(0), last_is_charging(false), cur_folder_path(""), launch_fail_warn_shown(false), min_alpha(min_alpha), target_alpha(0), mode(SuspendedImageMode::ShowingAfterStart), suspended_screen_alpha(0xFF) {
        // TODONEW: like nxlink but for sending themes and quickly being able to test them?
        
        if(captured_screen_buf != nullptr) {
            this->suspended_screen_img = RawRgbaImage::New(0, 0, captured_screen_buf, 1280, 720, 4);
        }
        else {
            this->suspended_screen_img = {};
        }

        // Load banners first
        this->top_menu_img = pu::ui::elm::Image::New(40, 35, TryFindImage(g_Theme, "ui/TopMenu"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "top_menu_bg", this->top_menu_img);
        this->Add(this->top_menu_img);

        // Then load buttons and other UI elements
        this->logo_img = ClickableImage::New(610, 13 + 35, "romfs:/Logo.png");
        this->logo_img->SetWidth(60);
        this->logo_img->SetHeight(60);
        this->logo_img->SetOnClick(&ShowAboutDialog);
        g_MenuApplication->ApplyConfigForElement("main_menu", "logo_icon", this->logo_img, false); // Sorry theme makers... uLaunch's logo must be visible, but can be moved
        this->Add(this->logo_img);

        this->connection_icon = pu::ui::elm::Image::New(80, 53, TryFindImage(g_Theme, "ui/NoConnectionIcon"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "connection_icon", this->connection_icon);
        this->Add(this->connection_icon);

        this->users_img = ClickableImage::New(270, 53, ""); // On layout creation, no user is still selected...
        this->users_img->SetOnClick(&ShowUserMenu);
        g_MenuApplication->ApplyConfigForElement("main_menu", "user_icon", this->users_img);
        this->Add(this->users_img);

        this->controller_img = ClickableImage::New(340, 53, TryFindImage(g_Theme, "ui/ControllerIcon"));
        this->controller_img->SetOnClick(&ShowControllerSupport);
        g_MenuApplication->ApplyConfigForElement("main_menu", "controller_icon", this->controller_img);
        this->Add(this->controller_img);

        this->time_text = pu::ui::elm::TextBlock::New(515, 65, "...");
        this->time_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "time_text", this->time_text);
        this->Add(this->time_text);

        this->battery_text = pu::ui::elm::TextBlock::New(700, 55, "...");
        this->battery_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->battery_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "battery_text", this->battery_text);
        this->Add(this->battery_text);

        this->battery_icon = pu::ui::elm::Image::New(700, 80, TryFindImage(g_Theme, "ui/BatteryNormalIcon"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "battery_icon", this->battery_icon);
        this->Add(this->battery_icon);

        this->settings_img = ClickableImage::New(810, 53, TryFindImage(g_Theme, "ui/SettingsIcon"));
        this->settings_img->SetOnClick(&ShowSettingsMenu);
        g_MenuApplication->ApplyConfigForElement("main_menu", "settings_icon", this->settings_img);
        this->Add(this->settings_img);

        this->themes_img = ClickableImage::New(880, 53, TryFindImage(g_Theme, "ui/ThemesIcon"));
        this->themes_img->SetOnClick(&ShowThemesMenu);
        g_MenuApplication->ApplyConfigForElement("main_menu", "themes_icon", this->themes_img);
        this->Add(this->themes_img);

        this->fw_text = pu::ui::elm::TextBlock::New(970, 65, g_MenuApplication->GetStatus().fw_version);
        this->fw_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "firmware_text", this->fw_text);
        this->Add(this->fw_text);

        this->input_bar = InputBar::New(0, 120);
        g_MenuApplication->ApplyConfigForElement("main_menu", "input_bar", this->input_bar);
        this->Add(this->input_bar);

        this->cur_path_text = pu::ui::elm::TextBlock::New(10, 170, this->cur_folder_path);
        this->cur_path_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->cur_path_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "cur_path_text", this->cur_path_text);
        this->Add(this->cur_path_text);

        this->banner_img = pu::ui::elm::Image::New(0, 585, TryFindImage(g_Theme, "ui/BannerInstalled"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_image", this->banner_img);

        this->cur_entry_name_text = pu::ui::elm::TextBlock::New(40, 610, "...");
        this->cur_entry_name_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
        this->cur_entry_name_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_name_text", this->cur_entry_name_text);

        this->cur_entry_author_text = pu::ui::elm::TextBlock::New(45, 650, "...");
        this->cur_entry_author_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->cur_entry_author_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_author_text", this->cur_entry_author_text);

        this->cur_entry_version_text = pu::ui::elm::TextBlock::New(45, 675, "...");
        this->cur_entry_version_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->cur_entry_version_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_version_text", this->cur_entry_version_text);

        this->no_entries_text = pu::ui::elm::TextBlock::New(0, 0, "No entries here");
        this->no_entries_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->no_entries_text->SetColor(g_MenuApplication->GetTextColor());
        this->no_entries_text->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->no_entries_text->SetVerticalAlign(pu::ui::elm::VerticalAlign::Center);
        this->no_entries_text->SetVisible(false);
        g_MenuApplication->ApplyConfigForElement("main_menu", "no_entries_text", this->no_entries_text);
        this->Add(this->no_entries_text);

        this->entries_menu = EntryMenu::New(0, 190, pu::ui::render::ScreenHeight - 190, g_MenuApplication->GetStatus().last_menu_path, g_MenuApplication->GetStatus().last_menu_index, std::bind(&MainMenuLayout::menu_EntryInputPressed, this, std::placeholders::_1), std::bind(&MainMenuLayout::menu_FocusedEntryChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        g_MenuApplication->ApplyConfigForElement("main_menu", "entries_menu", this->entries_menu);
        this->Add(this->entries_menu);

        this->Add(this->banner_img);

        this->Add(this->cur_entry_name_text);
        this->Add(this->cur_entry_author_text);
        this->Add(this->cur_entry_version_text);

        if(captured_screen_buf != nullptr) {
            this->Add(this->suspended_screen_img);
        }

        this->quick_menu = QuickMenu::New(TryFindImage(g_Theme, "ui/QuickMenuMain"));
        this->Add(this->quick_menu);

        this->startup_tp = std::chrono::steady_clock::now();

        this->title_launch_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/TitleLaunch.wav"));

        this->SetBackgroundImage(TryFindImage(g_Theme, "ui/Background"));

        this->entries_menu->Initialize();
    }

    MainMenuLayout::~MainMenuLayout() {
        pu::audio::DestroySfx(this->title_launch_sfx);
    }

    void MainMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        const auto quick_menu_on = this->quick_menu->IsOn();
        this->entries_menu->SetEnabled(!quick_menu_on);
        if(quick_menu_on) {
            return;
        }

        if(this->entries_menu->HasEntries()) {
            if(this->entries_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_move_selection"));
            }
            else if(this->entries_menu->IsFocusedEntrySuspended()) {
                this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_resume_suspended"));
            }
            else if(this->entries_menu->HasEntries()) {
                const auto &cur_entry = this->entries_menu->GetFocusedEntry();
                if(cur_entry.Is<EntryType::Folder>()) {
                    this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_open_folder"));
                }
                else {
                    this->input_bar->AddSetInput(HidNpadButton_A, GetLanguageString("input_launch_entry"));
                }
            }
            else {
                this->input_bar->AddSetInput(HidNpadButton_X, "");
            }

            if(this->entries_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_X, GetLanguageString("input_cancel_selection"));
            }
            else if(this->entries_menu->IsFocusedEntrySuspended()) {
                this->input_bar->AddSetInput(HidNpadButton_X, GetLanguageString("input_close_suspended"));
            }
            else if(this->entries_menu->HasEntries()) {
                this->input_bar->AddSetInput(HidNpadButton_X, GetLanguageString("input_entry_options"));
            }
            else {
                this->input_bar->AddSetInput(HidNpadButton_X, "");
            }

            this->input_bar->AddSetInput(HidNpadButton_Y, GetLanguageString("input_select_entry"));

            if(this->entries_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_B, GetLanguageString("input_cancel_selection"));
            }
            else if(!this->entries_menu->IsInRoot()) {
                this->input_bar->AddSetInput(HidNpadButton_B, GetLanguageString("input_folder_back"));
            }
            else {
                this->input_bar->AddSetInput(HidNpadButton_B, "");
            }

            this->input_bar->AddSetInput(HidNpadButton_L | HidNpadButton_R | HidNpadButton_ZL | HidNpadButton_ZR, "Quick menu");

            this->input_bar->AddSetInput(HidNpadButton_Plus | HidNpadButton_Minus, GetLanguageString("input_resize_menu"));

            this->input_bar->AddSetInput(HidNpadButton_StickL, GetLanguageString("input_new_entry"));
        }
        else {
            this->input_bar->ClearInputs();
        }

        const auto has_conn = net::HasConnection();
        if(this->last_has_connection != has_conn) {
            this->last_has_connection = has_conn;
            const auto conn_img = has_conn ? "ui/ConnectionIcon" : "ui/NoConnectionIcon";
            this->connection_icon->SetImage(TryFindImage(g_Theme, conn_img));
        }

        const auto cur_time = os::GetCurrentTime();
        this->time_text->SetText(cur_time);

        const auto battery_lvl = os::GetBatteryLevel();
        if(this->last_battery_lvl != battery_lvl) {
            this->last_battery_lvl = battery_lvl;
            const auto battery_str = std::to_string(battery_lvl) + "%";
            this->battery_text->SetText(battery_str);
        }

        const auto is_charging = os::IsConsoleCharging();
        if(this->last_is_charging != is_charging) {
            this->last_is_charging = is_charging;
            const auto battery_img = is_charging ? "ui/BatteryChargingIcon" : "ui/BatteryNormalIcon";
            this->battery_icon->SetImage(TryFindImage(g_Theme, battery_img));
        }

        const auto now_tp = std::chrono::steady_clock::now();
        // Wait a bit before handling sent messages
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now_tp - this->startup_tp).count() >= 1000) {
            if(g_MenuApplication->GetLaunchFailed()) {
                g_MenuApplication->CreateShowDialog(GetLanguageString("app_launch"), GetLanguageString("app_unexpected_error"), { GetLanguageString("ok") }, true);
            }
            else if(g_MenuApplication->HasChosenHomebrew()) {
                const auto nro_path = g_MenuApplication->GetChosenHomebrew();
                const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_chosen_hb"), GetLanguageString("menu_chosen_hb_info") + "\n\n" + nro_path + "\n\n" + GetLanguageString("menu_add_chosen_hb"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
                if(option == 0) {
                    // TODONEW: custom argv option?
                    CreateHomebrewEntry(this->entries_menu->GetPath(), nro_path, nro_path);
                    this->MoveTo("", true);
                    g_MenuApplication->ShowNotification(GetLanguageString("menu_chosen_hb_added"));
                }
            }
            else if(!g_CurrentThemeChecked) {
                if(cfg::IsThemeOutdated(g_Theme)) {
                    g_MenuApplication->CreateShowDialog(GetLanguageString("theme_current"), GetLanguageString("theme_outdated"), { GetLanguageString("ok") }, true);
                }
                g_CurrentThemeChecked = true;
            }
        }

        if(this->suspended_screen_img) {
            switch(this->mode) {
                case SuspendedImageMode::ShowingAfterStart: {
                    if(this->entries_menu->IsFocusedEntrySuspended() && (this->suspended_screen_alpha <= this->min_alpha)) {
                        this->suspended_screen_alpha = this->min_alpha;
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->mode = SuspendedImageMode::Focused;
                    }
                    else if(!this->entries_menu->IsFocusedEntrySuspended() && (this->suspended_screen_alpha == 0)) {
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

        if(keys_down & HidNpadButton_B) {
            if(!this->entries_menu->IsInRoot()) {
                const auto parent_path = fs::GetBaseDirectory(this->entries_menu->GetPath());
                this->PopFolder();
                this->MoveTo(parent_path, true);
            }
        }
        else if(keys_down & HidNpadButton_Plus) {
            this->entries_menu->IncrementHorizontalCount();
        }
        else if(keys_down & HidNpadButton_Minus) {
            this->entries_menu->DecrementHorizontalCount();
        }
    }

    bool MainMenuLayout::OnHomeButtonPress() {
        if(g_MenuApplication->IsSuspended()) {
            if(this->mode == SuspendedImageMode::Focused) {
                this->mode = SuspendedImageMode::HidingForResume;
            }
        }
        else {
            this->entries_menu->RewindEntries();
        }
        return true;
    }

    void MainMenuLayout::MoveTo(const std::string &new_path, const bool fade, std::function<void()> action) {
        if(fade) {
            g_TransitionGuard.Run([&]() {
                g_MenuApplication->FadeOut();

                if(action) {
                    action();
                }
                this->DoMoveTo(new_path);

                g_MenuApplication->FadeIn();
            });
        }
        else {
            this->DoMoveTo(new_path);
        }
    }

    void MainMenuLayout::SetUser(const AccountUid user) {
        this->users_img->SetImage(acc::GetIconCacheImagePath(user));
        this->users_img->SetWidth(50);
        this->users_img->SetHeight(50);
    }

    void MainMenuLayout::HandleCloseSuspended() {
        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("suspended_app"), GetLanguageString("suspended_close"), { GetLanguageString("yes"), GetLanguageString("no") }, true);
        if(option == 0) {
            this->DoTerminateApplication();
        }
    }

    void MainMenuLayout::HandleHomebrewLaunch(const Entry &hb_entry) {
        u64 app_takeover_id;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, app_takeover_id));
        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("hb_launch"), GetLanguageString("hb_launch_conf"), { GetLanguageString("hb_applet"), GetLanguageString("hb_app"), GetLanguageString("cancel") }, true);
        if(option == 0) {
            pu::audio::PlaySfx(this->title_launch_sfx);
            
            const auto proper_ipt = CreateLaunchTargetInput(hb_entry.hb_info.nro_target);
            UL_RC_ASSERT(smi::LaunchHomebrewLibraryApplet(proper_ipt.nro_path, proper_ipt.nro_argv));

            g_MenuApplication->StopPlayBGM();
            g_MenuApplication->CloseWithFadeOut();
        }
        else if(option == 1) {
            if(app_takeover_id != 0) {
                auto launch = true;
                if(g_MenuApplication->IsSuspended()) {
                    launch = false;
                    this->HandleCloseSuspended();
                    if(!g_MenuApplication->IsSuspended()) {
                        launch = true;
                    }
                }
                if(launch) {
                    pu::audio::PlaySfx(this->title_launch_sfx);
                    
                    const auto ipt = CreateLaunchTargetInput(hb_entry.hb_info.nro_target);
                    const auto rc = smi::LaunchHomebrewApplication(ipt.nro_path, ipt.nro_argv);

                    if(R_SUCCEEDED(rc)) {
                        g_MenuApplication->StopPlayBGM();
                        g_MenuApplication->CloseWithFadeOut();
                        return;
                    }
                    else {
                        g_MenuApplication->ShowNotification(GetLanguageString("app_launch_error") + ": " + util::FormatResultDisplay(rc));
                    }
                }
            }
            else {
                g_MenuApplication->CreateShowDialog(GetLanguageString("app_launch"), GetLanguageString("app_no_take_over_title") + "\n" + GetLanguageString("app_take_over_title_select"), { GetLanguageString("ok") }, true);
            }
        }
    }

    void MainMenuLayout::StopSelection() {
        this->entries_menu->ResetSelection();
    }

    void MainMenuLayout::DoTerminateApplication() {
        u32 i = 0;
        for(auto &entry : this->entries_menu->GetEntries()) {
            if(g_MenuApplication->IsEntrySuspended(entry)) {
                break;
            }
            i++;
        }

        UL_RC_ASSERT(smi::TerminateApplication());

        // We need to reload the application record
        // Its kind/type changed after closing it
        this->entries_menu->GetEntries().at(i).ReloadApplicationInfo();

        g_MenuApplication->ResetSuspendedApplication();

        this->mode = SuspendedImageMode::NotFocused;
        if(this->suspended_screen_img) {
            this->suspended_screen_img->SetAlpha(0);
        }
    }

}