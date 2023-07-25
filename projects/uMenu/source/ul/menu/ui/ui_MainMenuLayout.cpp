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

    }

    void MainMenuLayout::DoMoveTo(const std::string &new_path) {
        // Empty path used as a "reload" argumnt
        if(!new_path.empty()) {
            char menu_path[FS_MAX_PATH];
            util::CopyToStringBuffer(menu_path, new_path);
            UL_RC_ASSERT(smi::UpdateMenuPath(menu_path));
        }

        this->entries_menu->MoveTo(new_path);

        // TODONEW: remove this when the cwd is properly displayed somehow
        this->fw_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small));
        this->fw_text->SetText(this->entries_menu->GetPath());

        if(this->entries_menu->HasEntries()) {
            this->banner_img->SetVisible(true);
            this->selected_item_name_text->SetVisible(true);
            this->selected_item_author_text->SetVisible(true);
            this->selected_item_version_text->SetVisible(true);
            this->no_entries_text->SetVisible(false);

            /*
            u32 cur_i = 0;
            for(const auto &entry : g_CurrentMenuEntries) {
                auto entry_icon = entry.control.icon_path;
                if(entry_icon.empty() && entry.Is<EntryType::Folder>()) {
                    entry_icon = cfg::GetAssetByTheme(g_Theme, "ui/Folder.png");
                }
                this->entries_menu->AddItem(entry_icon);

                if((entry.Is<EntryType::Application>() && (g_MenuApplication->GetStatus().suspended_app_id == entry.app_info.record.application_id)) || (entry.Is<EntryType::Homebrew>() && g_MenuApplication->EqualsSuspendedHomebrewPath(entry.hb_info.nro_target.nro_path))) {
                    this->entries_menu->SetSuspendedItem(cur_i);
                }

                cur_i++;
            }

            // TODONEW: find a faster way to do this?
            g_MenuApplication->CallForRender();
            if(!g_EntryIndexStack.empty()) {
                const auto saved_i = g_EntryIndexStack.top();
                if(saved_i < g_CurrentMenuEntries.size()) {
                    while(this->entries_menu->GetSelectedItem() < saved_i) {
                        this->entries_menu->HandleMoveRight();
                    }
                }
            }
            */
        }
        else {
            this->banner_img->SetVisible(false);
            this->selected_item_name_text->SetVisible(false);
            this->selected_item_author_text->SetVisible(false);
            this->selected_item_version_text->SetVisible(false);
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
                const auto option = g_MenuApplication->CreateShowDialog("Selection", "Would you like to cancel the current selection?", { "Yes", "Cancel" }, true);
                if(option == 0) {
                    // TODONEW: (MULTI)select, strings, standfn
                    g_MenuApplication->ShowNotification(GetLanguageString("menu_multiselect_cancel"));
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
                        g_MenuApplication->ShowNotification("Cant move the folder into itself!");
                    }
                    else {
                        const auto option = g_MenuApplication->CreateShowDialog("Multiselect", "Would you like to move all to this dir?", { "Yes", "Cancel" }, true);
                        if(option == 0) {
                            u32 cur_i = 0;
                            for(auto &entry : this->entries_menu->GetEntries()) {
                                if(this->entries_menu->IsEntrySelected(cur_i)) {
                                    entry.MoveTo(cur_entry.GetFolderPath());
                                }

                                cur_i++;
                            }

                            this->StopSelection();
                            g_MenuApplication->ShowNotification("all moved");
                            this->MoveTo("", true);
                        }
                    }
                }
                else {
                    auto &cur_entries = this->entries_menu->GetEntries();
                    const auto cur_i = this->entries_menu->GetFocusedEntryIndex();
                    const auto last_i = cur_entries.size() - 1;

                    // TODONEW: proper strings
                    // TODONEW: apply this to folder too? maybe pick better key inputs for this
                    const auto option = g_MenuApplication->CreateShowDialog("Multiselect", "Move stuff kind", { "Before", "After", "Swap", "Cancel" }, true);
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
                        g_MenuApplication->ShowNotification("all moved before");
                        this->MoveTo("", true);
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
                        g_MenuApplication->ShowNotification("all moved after");
                        this->MoveTo("", true);
                    }
                    else if(option == 2) {
                        // Basically a "move before" followed by moving the current item in the range where the first multiselected item was
                        auto cur_start_idx = (cur_i == 0) ? InvalidEntryIndex : cur_entries.at(cur_i - 1).index;
                        const auto cur_end_idx = cur_entry.index;
                        u32 cur_i = 0;
                        u32 move_start_idx = UINT32_MAX;
                        u32 move_end_idx = UINT32_MAX;
                        for(auto &entry : cur_entries) {
                            if(this->entries_menu->IsEntrySelected(cur_i)) {
                                // This way, get the start+end index which map the range of the first multiselected item
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
                        g_MenuApplication->ShowNotification("all swapped with one");
                        this->MoveTo("", true);
                    }
                }
            }
            else {
                if(cur_entry.Is<EntryType::Folder>()) {
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
                        // TODONEW: gamecard detection, proper strings, etc
                        g_MenuApplication->ShowNotification("Not launchable (gamecard not inserted, archived, not downloaded, etc)");
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
            g_MenuApplication->ShowNotification("fucking toggle: " + std::to_string(this->entries_menu->IsFocusedEntrySelected()));
        }
        else if(keys_down & HidNpadButton_X) {
            if(this->entries_menu->IsAnySelected()) {
                const auto option = g_MenuApplication->CreateShowDialog("Selection", "Would you like to cancel the current selection?", { "Yes", "Cancel" }, true);
                if(option == 0) {
                    // TODONEW: above
                    g_MenuApplication->ShowNotification(GetLanguageString("menu_multiselect_cancel"));
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
                        // TODONEW: confirm?
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
                        const auto option_2 = g_MenuApplication->CreateShowDialog(GetLanguageString("entry_remove"), GetLanguageString("entry_remove_conf"), { GetLanguageString("yes"), GetLanguageString("no") }, true);
                        if(option_2 == 0) {
                            if(strcmp(cur_entry.hb_info.nro_target.nro_path, ul::HbmenuPath) == 0) {
                                g_MenuApplication->ShowNotification("hbmenu not removable");
                            }
                            else {
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

            const auto option = g_MenuApplication->CreateShowDialog("New options", "Choose option", { "New folder", "Add homebrew to menu", "Cancel" }, true);
            if(option == 0) {
                SwkbdConfig cfg;
                // TODONEW: check results here
                swkbdCreate(&cfg, 0);
                swkbdConfigSetGuideText(&cfg, "New folder name");
                char new_folder_name[500] = {};
                const auto rc = swkbdShow(&cfg, new_folder_name, sizeof(new_folder_name));
                swkbdClose(&cfg);
                if(R_SUCCEEDED(rc)) {
                    CreateFolderEntry(this->entries_menu->GetPath(), new_folder_name);
                    g_MenuApplication->ShowNotification("Folder created");
                    this->MoveTo("", true);
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

        this->selected_item_author_text->SetVisible(true);
        this->selected_item_version_text->SetVisible(true);

        UL_RC_ASSERT(smi::UpdateMenuIndex(this->entries_menu->GetFocusedEntryIndex()));

        auto &cur_entry = this->entries_menu->GetFocusedEntry();
        if(cur_entry.Is<EntryType::Folder>()) {
            this->banner_img->SetImage(cfg::GetAssetByTheme(g_Theme, "ui/BannerFolder.png"));

            // TODONEW: entry count?
            // this->selected_item_author_text->SetText(std::to_string(folder_entry_count) + " " + ((folder_entry_count == 1) ? GetLanguageString("folder_entry_single") : GetLanguageString("folder_entry_mult")));
            this->selected_item_name_text->SetText(cur_entry.folder_info.name);
            this->selected_item_author_text->SetText(cur_entry.folder_info.fs_name);
            this->selected_item_version_text->SetVisible(false);
        }
        else {
            if(cur_entry.Is<EntryType::Application>()) {
                this->banner_img->SetImage(cfg::GetAssetByTheme(g_Theme, "ui/BannerInstalled.png"));
            }
            else {
                this->banner_img->SetImage(cfg::GetAssetByTheme(g_Theme, "ui/BannerHomebrew.png"));
            }
            
            cur_entry.TryLoadControlData();
            if(cur_entry.control.IsLoaded()) {
                if(cur_entry.Is<EntryType::Application>()) {
                    // TODONEW: remove this debug params when no longer necessary
                    this->selected_item_name_text->SetText(cur_entry.control.name + " (type " + std::to_string(cur_entry.app_info.record.type) + ", stid + " + std::to_string(cur_entry.app_info.meta_status.storageID) + ")");
                }
                else {
                    this->selected_item_name_text->SetText(cur_entry.control.name);
                }

                /*
                this->selected_item_author_text->SetText(cur_entry.control.author);
                this->selected_item_version_text->SetText(cur_entry.control.version);
                */

                this->selected_item_author_text->SetText(cur_entry.entry_path);
                this->selected_item_version_text->SetText(std::to_string(cur_entry.index));
            }
            else {
                // TODONEW: what to show when controldata doesnt load? (really shouldnt happen anyway)
                this->selected_item_name_text->SetText("Unknown name...");
                this->selected_item_author_text->SetText("Unknown author...");
                this->selected_item_version_text->SetText("Unknown version...");
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

    MainMenuLayout::MainMenuLayout(const u8 *captured_screen_buf, const u8 min_alpha) : last_has_connection(false), last_battery_lvl(0), last_is_charging(false), launch_fail_warn_shown(false), min_alpha(min_alpha), target_alpha(0), mode(SuspendedImageMode::ShowingAfterStart), suspended_screen_alpha(0xFF) {
        // const auto menu_text_x = g_MenuApplication->GetUIConfigValue<u32>("menu_folder_text_x", 30);
        // const auto menu_text_y = g_MenuApplication->GetUIConfigValue<u32>("menu_folder_text_y", 200);
        // const auto menu_text_size = g_MenuApplication->GetUIConfigValue<u32>("menu_folder_text_size", 25);

        if(captured_screen_buf != nullptr) {
            this->suspended_screen_img = RawRgbaImage::New(0, 0, captured_screen_buf, 1280, 720, 4);
        }
        else {
            this->suspended_screen_img = {};
        }

        // Load banners first
        this->top_menu_img = pu::ui::elm::Image::New(40, 35, cfg::GetAssetByTheme(g_Theme, "ui/TopMenu.png"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "top_menu_bg", this->top_menu_img);
        this->Add(this->top_menu_img);

        // Then load buttons and other UI elements
        this->logo_img = ClickableImage::New(610, 13 + 35, "romfs:/Logo.png");
        this->logo_img->SetWidth(60);
        this->logo_img->SetHeight(60);
        this->logo_img->SetOnClick(&ShowAboutDialog);
        g_MenuApplication->ApplyConfigForElement("main_menu", "logo_icon", this->logo_img, false); // Sorry theme makers... uLaunch's logo must be visible, but can be moved
        this->Add(this->logo_img);

        this->connection_icon = pu::ui::elm::Image::New(80, 53, cfg::GetAssetByTheme(g_Theme, "ui/NoConnectionIcon.png"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "connection_icon", this->connection_icon);
        this->Add(this->connection_icon);

        this->users_img = ClickableImage::New(270, 53, ""); // On layout creation, no user is still selected...
        this->users_img->SetOnClick(&ShowUserMenu);
        g_MenuApplication->ApplyConfigForElement("main_menu", "user_icon", this->users_img);
        this->Add(this->users_img);

        this->controller_img = ClickableImage::New(340, 53, cfg::GetAssetByTheme(g_Theme, "ui/ControllerIcon.png"));
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

        this->battery_icon = pu::ui::elm::Image::New(700, 80, cfg::GetAssetByTheme(g_Theme, "ui/BatteryNormalIcon.png"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "battery_icon", this->battery_icon);
        this->Add(this->battery_icon);

        this->settings_img = ClickableImage::New(810, 53, cfg::GetAssetByTheme(g_Theme, "ui/SettingsIcon.png"));
        this->settings_img->SetOnClick(&ShowSettingsMenu);
        g_MenuApplication->ApplyConfigForElement("main_menu", "settings_icon", this->settings_img);
        this->Add(this->settings_img);

        this->themes_img = ClickableImage::New(880, 53, cfg::GetAssetByTheme(g_Theme, "ui/ThemesIcon.png"));
        this->themes_img->SetOnClick(&ShowThemesMenu);
        g_MenuApplication->ApplyConfigForElement("main_menu", "themes_icon", this->themes_img);
        this->Add(this->themes_img);

        this->fw_text = pu::ui::elm::TextBlock::New(970, 65, g_MenuApplication->GetStatus().fw_version);
        this->fw_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "firmware_text", this->fw_text);
        this->Add(this->fw_text);

        this->input_bar = InputBar::New(120);
        this->Add(this->input_bar);

        this->banner_img = pu::ui::elm::Image::New(0, 585, cfg::GetAssetByTheme(g_Theme, "ui/BannerInstalled.png"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_image", this->banner_img);

        this->selected_item_name_text = pu::ui::elm::TextBlock::New(40, 610, "...");
        this->selected_item_name_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
        this->selected_item_name_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_name_text", this->selected_item_name_text);

        this->selected_item_author_text = pu::ui::elm::TextBlock::New(45, 650, "...");
        this->selected_item_author_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->selected_item_author_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_author_text", this->selected_item_author_text);

        this->selected_item_version_text = pu::ui::elm::TextBlock::New(45, 675, "...");
        this->selected_item_version_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->selected_item_version_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_version_text", this->selected_item_version_text);

        this->no_entries_text = pu::ui::elm::TextBlock::New(0, 0, "No entries here");
        this->no_entries_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->no_entries_text->SetColor(g_MenuApplication->GetTextColor());
        this->no_entries_text->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->no_entries_text->SetVerticalAlign(pu::ui::elm::VerticalAlign::Center);
        this->no_entries_text->SetVisible(false);
        // TODONEW: g_MenuApplication->ApplyConfigForElement("main_menu", "banner_author_text", this->no_entries_text);
        this->Add(this->no_entries_text);

        this->entries_menu = EntryMenu::New(160, pu::ui::render::ScreenHeight - 160, g_MenuApplication->GetStatus().last_menu_path, g_MenuApplication->GetStatus().last_menu_index, std::bind(&MainMenuLayout::menu_EntryInputPressed, this, std::placeholders::_1), std::bind(&MainMenuLayout::menu_FocusedEntryChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)); // SideMenu::New(pu::ui::Color(0, 255, 120, 0xFF), cfg::GetAssetByTheme(g_Theme, "ui/Cursor.png"), cfg::GetAssetByTheme(g_Theme, "ui/Suspended.png"), cfg::GetAssetByTheme(g_Theme, "ui/Multiselect.png"), menu_text_x, menu_text_y, pu::ui::MakeDefaultFontName(menu_text_size), g_MenuApplication->GetTextColor(), 294);
        this->Add(this->entries_menu);

        this->Add(this->banner_img);

        this->Add(this->selected_item_name_text);
        this->Add(this->selected_item_author_text);
        this->Add(this->selected_item_version_text);

        if(captured_screen_buf != nullptr) {
            this->Add(this->suspended_screen_img);
        }

        this->quick_menu = QuickMenu::New(cfg::GetAssetByTheme(g_Theme, "ui/QuickMenuMain.png"));
        this->Add(this->quick_menu);

        this->startup_tp = std::chrono::steady_clock::now();

        this->title_launch_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/TitleLaunch.wav"));

        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));

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
                this->input_bar->AddSetInput(HidNpadButton_A, "Move selection");
            }
            else if(this->entries_menu->IsFocusedEntrySuspended()) {
                this->input_bar->AddSetInput(HidNpadButton_A, "Resume");
            }
            else if(this->entries_menu->HasEntries()) {
                const auto &cur_entry = this->entries_menu->GetFocusedEntry();
                if(cur_entry.Is<EntryType::Folder>()) {
                    this->input_bar->AddSetInput(HidNpadButton_A, "Open");
                }
                else {
                    this->input_bar->AddSetInput(HidNpadButton_A, "Launch");
                }
            }
            else {
                this->input_bar->AddSetInput(HidNpadButton_X, "");
            }

            if(this->entries_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_X, "Cancel selection");
            }
            else if(this->entries_menu->IsFocusedEntrySuspended()) {
                this->input_bar->AddSetInput(HidNpadButton_X, "Close");
            }
            else if(this->entries_menu->HasEntries()) {
                this->input_bar->AddSetInput(HidNpadButton_X, "Options");
            }
            else {
                this->input_bar->AddSetInput(HidNpadButton_X, "");
            }

            this->input_bar->AddSetInput(HidNpadButton_Y, "Select");

            if(this->entries_menu->IsAnySelected()) {
                this->input_bar->AddSetInput(HidNpadButton_B, "Cancel selection");
            }
            else if(!this->entries_menu->IsInRoot()) {
                this->input_bar->AddSetInput(HidNpadButton_B, "Go back");
            }
            else {
                this->input_bar->AddSetInput(HidNpadButton_B, "");
            }

            this->input_bar->AddSetInput(HidNpadButton_L | HidNpadButton_R | HidNpadButton_ZL | HidNpadButton_ZR, "Quick menu");

            this->input_bar->AddSetInput(HidNpadButton_Plus | HidNpadButton_Minus, "Resize");

            this->input_bar->AddSetInput(HidNpadButton_StickL, "New entry");
        }
        else {
            this->input_bar->ClearInputs();
        }

        const auto has_conn = net::HasConnection();
        if(this->last_has_connection != has_conn) {
            this->last_has_connection = has_conn;
            const auto conn_img = has_conn ? "ui/ConnectionIcon.png" : "ui/NoConnectionIcon.png";
            this->connection_icon->SetImage(cfg::GetAssetByTheme(g_Theme, conn_img));
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
            const auto battery_img = is_charging ? "ui/BatteryChargingIcon.png" : "ui/BatteryNormalIcon.png";
            this->battery_icon->SetImage(cfg::GetAssetByTheme(g_Theme, battery_img));
        }

        const auto now_tp = std::chrono::steady_clock::now();
        // Wait a bit before handling sent messages
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now_tp - this->startup_tp).count() >= 1000) {
            if(g_MenuApplication->GetLaunchFailed()) {
                g_MenuApplication->CreateShowDialog(GetLanguageString("app_launch"), GetLanguageString("app_unexpected_error"), { GetLanguageString("ok") }, true);
            }
            else if(g_MenuApplication->HasChosenHomebrew()) {
                const auto nro_path = g_MenuApplication->GetChosenHomebrew();
                g_MenuApplication->CreateShowDialog("chosen hb", nro_path, { "K" }, true);

                // TODONEW: custom argv option?
                CreateHomebrewEntry(this->entries_menu->GetPath(), nro_path, nro_path);
                g_MenuApplication->ShowNotification("Homebrew added");
                this->MoveTo("", true);
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
                this->MoveTo(parent_path, true);
            }
        }
        else if(keys_down & HidNpadButton_Plus) {
            // ShowAboutDialog();
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
        u64 title_takeover_id;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, title_takeover_id));
        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("hb_launch"), GetLanguageString("hb_launch_conf"), { GetLanguageString("hb_applet"), GetLanguageString("hb_app"), GetLanguageString("cancel") }, true);
        if(option == 0) {
            pu::audio::PlaySfx(this->title_launch_sfx);
            
            const auto proper_ipt = CreateLaunchTargetInput(hb_entry.hb_info.nro_target);
            UL_RC_ASSERT(smi::LaunchHomebrewLibraryApplet(proper_ipt.nro_path, proper_ipt.nro_argv));

            g_MenuApplication->StopPlayBGM();
            g_MenuApplication->CloseWithFadeOut();
        }
        else if(option == 1) {
            if(title_takeover_id != 0) {
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

    void MainMenuLayout::HandleMultiselectMoveToFolder(const std::string &new_path) {
        if(this->entries_menu->IsAnySelected()) {
            u32 cur_i = 0;
            for(auto &entry : this->entries_menu->GetEntries()) {
                if(this->entries_menu->IsEntrySelected(cur_i)) {
                    entry.MoveTo(new_path);
                }

                cur_i++;
            }

            this->StopSelection();
            this->MoveTo("", true);
            // TODONEW: proper string
            g_MenuApplication->ShowNotification("Move was done");
        }
    }

    void MainMenuLayout::StopSelection() {
        this->entries_menu->ResetSelection();
    }

    void MainMenuLayout::DoTerminateApplication() {
        UL_RC_ASSERT(smi::TerminateApplication());

        g_MenuApplication->ResetSuspendedApplication();
        
        if(this->suspended_screen_img) {
            this->suspended_screen_img->SetAlpha(0);
        }
    }

}