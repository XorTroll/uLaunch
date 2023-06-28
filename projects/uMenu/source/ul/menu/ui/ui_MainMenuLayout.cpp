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
extern ul::cfg::TitleList g_EntryList;
extern std::vector<ul::cfg::TitleRecord> g_HomebrewRecordList;
extern ul::cfg::Config g_Config;
extern ul::cfg::Theme g_Theme;
extern char g_FwVersion[0x18];

#define MENU_HBMENU_NRO "sdmc:/hbmenu.nro"

namespace ul::menu::ui {

    namespace {

        inline loader::TargetInput CreateLaunchTargetInput(const loader::TargetInput &base_params) {
            loader::TargetInput ipt = {};
            memcpy(ipt.nro_path, base_params.nro_path, sizeof(ipt.nro_path));
            if(strlen(base_params.nro_argv)) {
                const auto default_argv = std::string(base_params.nro_path) + " " + base_params.nro_argv;
                strncpy(ipt.nro_argv, default_argv.c_str(), default_argv.length());
            }
            else {
                memcpy(ipt.nro_argv, base_params.nro_path, sizeof(ipt.nro_argv));
            }
            return ipt;
        }

    }

    void MainMenuLayout::DoMoveFolder(const std::string &name) {
        if(this->homebrew_mode) {
            if(g_HomebrewRecordList.empty()) {
                g_HomebrewRecordList = cfg::QueryAllHomebrew();
            }
        }

        auto item_list = g_HomebrewRecordList;
        if(!this->homebrew_mode) {
            const auto &folder = cfg::FindFolderByName(g_EntryList, name);
            item_list = folder.titles;
        }

        this->items_menu->ClearItems();
        if(this->homebrew_mode) {
            this->items_menu->AddItem(cfg::GetAssetByTheme(g_Theme, "ui/Hbmenu.png"));
        }
        else {
            if(name.empty()) {
                // Remove empty folders
                UL_STL_REMOVE_IF(g_EntryList.folders, folder, folder.titles.empty());
                for(const auto &folder: g_EntryList.folders) {
                    this->items_menu->AddItem(cfg::GetAssetByTheme(g_Theme, "ui/Folder.png"), folder.name);
                }
            }
        }

        u32 tmp_idx = 0;
        for(const auto &item: item_list) {
            auto set_susp = false;
            if(item.title_type == cfg::TitleType::Application) {
                if(g_MenuApplication->IsTitleSuspended()) {
                    if(g_MenuApplication->GetSuspendedApplicationId() == item.app_info.record.application_id) {
                        set_susp = true;
                    }
                }
            }
            else {
                if(g_MenuApplication->IsHomebrewSuspended()) {
                    if(g_MenuApplication->EqualsSuspendedHomebrewPath(item.hb_info.nro_target.nro_path)) {
                        set_susp = true;
                    }
                }
            }
            this->items_menu->AddItem(cfg::GetRecordIconPath(item));
            if(set_susp) {
                u32 susp_idx = tmp_idx;
                // Skip initial item if homebrew mode
                if(this->homebrew_mode) {
                    susp_idx++;
                }
                // Ignore front folders from main menu
                else if(name.empty()) {
                    susp_idx += g_EntryList.folders.size();
                }
                this->items_menu->SetSuspendedItem(susp_idx);
            }
            tmp_idx++;
        }

        this->items_menu->UpdateBorderIcons();
        if(!this->homebrew_mode) {
            this->cur_folder = name;
        }
    }

    void MainMenuLayout::menu_Click(const u64 keys_down, const u32 idx) {
        if(this->select_on && (keys_down & HidNpadButton_A)) {
            if(!this->items_menu->IsAnyMultiselected()) {
                this->StopMultiselect();
            }
        }
        if(this->select_on) {
            if(select_dir) {
                if((keys_down & HidNpadButton_A) || (keys_down & HidNpadButton_Y)) {
                    if((!this->homebrew_mode) && this->cur_folder.empty()) {
                        if(idx < g_EntryList.folders.size()) {
                            auto &folder = g_EntryList.folders.at(idx);
                            const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_multiselect"), GetLanguageString("menu_move_existing_folder_conf"), { GetLanguageString("yes"), GetLanguageString("no"), GetLanguageString("cancel") }, true);
                            if(option == 0) {
                                this->HandleMultiselectMoveToFolder(folder.name);
                            }
                            else if(option == 1) {
                                this->StopMultiselect();
                            }
                        }
                    }
                }
                else if(keys_down & HidNpadButton_B) {
                    this->select_dir = false;
                    g_MenuApplication->ShowNotification(GetLanguageString("menu_move_select_folder_cancel"));
                }
            }
            else {
                if(keys_down & HidNpadButton_B) {
                    g_MenuApplication->ShowNotification(GetLanguageString("menu_multiselect_cancel"));
                    this->StopMultiselect();
                }
                else if(keys_down & HidNpadButton_Y) {
                    auto selectable = false;
                    if(this->homebrew_mode) {
                        selectable = true;
                    }
                    else {
                        if(this->cur_folder.empty()) {
                            selectable = idx >= g_EntryList.folders.size();
                        }
                        else {
                            selectable = true;
                        }
                    }
                    if(selectable) {
                        this->items_menu->SetItemMultiselected(idx, !this->items_menu->IsItemMultiselected(idx));
                    }
                }
                else if(keys_down & HidNpadButton_A) {
                    if(this->homebrew_mode) {
                        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_multiselect"), GetLanguageString("hb_mode_entries_add"), { GetLanguageString("yes"), GetLanguageString("no"), GetLanguageString("cancel") }, true);
                        if(option == 0) {
                            // Get the idx of the last g_HomebrewRecordList element.
                            s32 hb_idx = 0;
                            for(auto &entry: g_EntryList.root.titles) {
                                if(entry.title_type == cfg::TitleType::Application) {
                                    break;
                                }
                                hb_idx++;
                            }
                            if(hb_idx < 0) {
                                hb_idx = 0;
                            }
                            auto all_added = true;
                            for(u32 i = 0; i < g_HomebrewRecordList.size(); i++) {
                                auto &hb = g_HomebrewRecordList.at(i);
                                const auto idx = i + 1;
                                if(this->items_menu->IsItemMultiselected(idx)) {
                                    if(!cfg::ExistsRecord(g_EntryList, hb)) {
                                        cfg::SaveRecord(hb);
                                        g_EntryList.root.titles.insert(g_EntryList.root.titles.begin() + hb_idx, hb);
                                        hb_idx++;
                                    }
                                    else {
                                        all_added = false;
                                    }
                                }
                            }
                            if(all_added) {
                                g_MenuApplication->ShowNotification(GetLanguageString("hb_mode_entries_added"));
                            }
                            else {
                                g_MenuApplication->ShowNotification(GetLanguageString("hb_mode_entries_some_added"));
                            }
                            this->StopMultiselect();
                        }
                        else if(option == 1) {
                            this->StopMultiselect();
                        }
                    }
                    else if(this->cur_folder.empty()) {
                        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_multiselect"), GetLanguageString("menu_move_to_folder"), { GetLanguageString("menu_move_new_folder"), GetLanguageString("menu_move_existing_folder"), GetLanguageString("no"), GetLanguageString("cancel") }, true);
                        if(option == 0) {
                            SwkbdConfig cfg;
                            swkbdCreate(&cfg, 0);
                            swkbdConfigSetGuideText(&cfg, GetLanguageString("swkbd_new_folder_guide").c_str());
                            char dir[500] = {};
                            const auto rc = swkbdShow(&cfg, dir, sizeof(dir));
                            swkbdClose(&cfg);
                            if(R_SUCCEEDED(rc)) {
                                this->HandleMultiselectMoveToFolder(dir);
                            }
                        }
                        else if(option == 1) {
                            this->select_dir = true;
                            g_MenuApplication->ShowNotification(GetLanguageString("menu_move_select_folder"));
                        }
                        else if(option == 2) {
                            this->StopMultiselect();
                        }
                    }
                    else {
                        auto sopt = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_multiselect"), GetLanguageString("menu_move_from_folder"), { GetLanguageString("yes"), GetLanguageString("no"), GetLanguageString("cancel") }, true);
                        if(sopt == 0) {
                            u32 removed_idx = 0;
                            auto &folder = cfg::FindFolderByName(g_EntryList, this->cur_folder);
                            for(u32 i = 0; i < folder.titles.size(); i++) {
                                auto &title = folder.titles.at(i - removed_idx);
                                if(this->items_menu->IsItemMultiselected(i)) {
                                    if(cfg::MoveRecordTo(g_EntryList, title, "")) {
                                        removed_idx++;
                                    }
                                }
                            }
                            this->StopMultiselect();
                            this->MoveFolder(folder.titles.empty() ? "" : this->cur_folder, true);
                        }
                        else if(sopt == 1) {
                            this->StopMultiselect();
                        }
                    }
                }
            }
        }
        else {
            if((idx == 0) && this->homebrew_mode) {
                if(keys_down & HidNpadButton_A) {
                    pu::audio::PlaySfx(this->title_launch_sfx);
                    
                    // Launch normal hbmenu
                    UL_RC_ASSERT(menu::smi::LaunchHomebrewLibraryApplet(MENU_HBMENU_NRO, MENU_HBMENU_NRO));

                    g_MenuApplication->StopPlayBGM();
                    g_MenuApplication->CloseWithFadeOut();
                    return;
                }
            }
            else {
                s32 actual_idx = idx;
                if(this->homebrew_mode) {
                    actual_idx--;
                    const auto &hb = g_HomebrewRecordList.at(actual_idx);
                    if(keys_down & HidNpadButton_A) {
                        auto launch_hb = true;
                        if(g_MenuApplication->IsHomebrewSuspended()) {
                            if(g_MenuApplication->EqualsSuspendedHomebrewPath(hb.hb_info.nro_target.nro_path)) {
                                if(this->mode == SuspendedImageMode::Focused) {
                                    this->mode = SuspendedImageMode::HidingForResume;
                                }
                                launch_hb = false;
                            }
                            else {
                                launch_hb = false;
                                this->HandleCloseSuspended();
                                launch_hb = !g_MenuApplication->IsHomebrewSuspended();
                            }
                        }
                        if(launch_hb) {
                            this->HandleHomebrewLaunch(hb);
                        }
                    }
                    else if(keys_down & HidNpadButton_X) {
                        if(g_MenuApplication->IsSuspended()) {
                            if(g_MenuApplication->EqualsSuspendedHomebrewPath(hb.hb_info.nro_target.nro_path)) {
                                this->HandleCloseSuspended();
                            }
                        }
                    }
                    else if(keys_down & HidNpadButton_Y) {
                        this->select_on = true;
                        this->items_menu->SetItemMultiselected(this->items_menu->GetSelectedItem(), true);
                    }
                }
                else {
                    auto &folder = cfg::FindFolderByName(g_EntryList, this->cur_folder);
                    s32 title_idx = actual_idx;
                    if(this->cur_folder.empty()) {
                        if(static_cast<u32>(actual_idx) >= g_EntryList.folders.size()) {
                            title_idx -= g_EntryList.folders.size();
                        }
                        else {
                            auto &selected_folder = g_EntryList.folders.at(actual_idx);
                            if(keys_down & HidNpadButton_A) {
                                this->MoveFolder(selected_folder.name, true);
                            }
                            else if(keys_down & HidNpadButton_Y) {
                                const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("menu_rename_folder"), GetLanguageString("menu_rename_folder_conf"), { GetLanguageString("yes"), GetLanguageString("no") }, true);
                                if(option == 0) {
                                    SwkbdConfig cfg;
                                    swkbdCreate(&cfg, 0);
                                    swkbdConfigSetGuideText(&cfg, GetLanguageString("swkbd_rename_folder_guide").c_str());
                                    char dir[500] = {0};
                                    const auto rc = swkbdShow(&cfg, dir, sizeof(dir));
                                    swkbdClose(&cfg);
                                    if(R_SUCCEEDED(rc)) {
                                        cfg::RenameFolder(g_EntryList, selected_folder.name, dir);
                                        this->MoveFolder(this->cur_folder, true);
                                    }
                                }
                            }
                            title_idx = -1;
                        }
                    }
                    if(title_idx >= 0) {
                        auto &title = folder.titles.at(title_idx);
                        if(keys_down & HidNpadButton_A) {
                            auto launch_title = true;

                            if(g_MenuApplication->IsSuspended()) {
                                if(title.title_type == cfg::TitleType::Homebrew) {
                                    if(g_MenuApplication->EqualsSuspendedHomebrewPath(title.hb_info.nro_target.nro_path)) {
                                        if(this->mode == SuspendedImageMode::Focused) {
                                            this->mode = SuspendedImageMode::HidingForResume;
                                        }
                                        launch_title = false;
                                    }
                                }
                                else if(title.title_type == cfg::TitleType::Application) {
                                    if(title.app_info.record.application_id == g_MenuApplication->GetSuspendedApplicationId()) {
                                        if(this->mode == SuspendedImageMode::Focused) {
                                            this->mode = SuspendedImageMode::HidingForResume;
                                        }
                                        launch_title = false;
                                    }
                                }
                                if(launch_title) {
                                    // Homebrew launching code already does this checks later - doing this check only with installed titles
                                    if(title.title_type == cfg::TitleType::Application) {
                                        launch_title = false;
                                        this->HandleCloseSuspended();
                                        launch_title = !g_MenuApplication->IsSuspended();
                                    }
                                }
                            }

                            if(title.title_type == cfg::TitleType::Application) {
                                if(!title.app_info.IsLaunchable()) {
                                    // TODONEW: gamecard detection
                                    g_MenuApplication->ShowNotification("Not launchable (gamecard not inserted, archived, not downloaded, etc)");
                                    launch_title = false;
                                }
                            }

                            if(launch_title) {
                                if(title.title_type == cfg::TitleType::Homebrew) {
                                    this->HandleHomebrewLaunch(title);
                                }
                                else {
                                    pu::audio::PlaySfx(this->title_launch_sfx);

                                    const auto rc = menu::smi::LaunchApplication(title.app_info.record.application_id);
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
                        else if(keys_down & HidNpadButton_X) {
                            if(g_MenuApplication->IsSuspended()) {
                                if(title.title_type == cfg::TitleType::Homebrew) {
                                    if(g_MenuApplication->EqualsSuspendedHomebrewPath(title.hb_info.nro_target.nro_path)) {
                                        this->HandleCloseSuspended();
                                    }
                                }
                                else {
                                    if(title.app_info.record.application_id == g_MenuApplication->GetSuspendedApplicationId()) {
                                        this->HandleCloseSuspended();
                                    }
                                }
                            }
                        }
                        else if(keys_down & HidNpadButton_Y) {
                            if(title.title_type == cfg::TitleType::Homebrew) {
                                const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("entry_options"), GetLanguageString("entry_action"), { GetLanguageString("entry_move"), GetLanguageString("entry_remove"), GetLanguageString("cancel") }, true);
                                if(option == 0) {
                                    if(!this->select_on) {
                                        this->select_on = true;
                                    }
                                    this->items_menu->SetItemMultiselected(this->items_menu->GetSelectedItem(), true);
                                }
                                else if(option == 1) {
                                    const auto option_2 = g_MenuApplication->CreateShowDialog(GetLanguageString("entry_remove"), GetLanguageString("entry_remove_conf"), { GetLanguageString("yes"), GetLanguageString("no") }, true);
                                    if(option_2 == 0) {
                                        cfg::RemoveRecord(title);
                                        folder.titles.erase(folder.titles.begin() + title_idx);
                                        g_MenuApplication->ShowNotification(GetLanguageString("entry_remove_ok"));
                                        this->MoveFolder(this->cur_folder, true);
                                    }
                                }
                            }
                            else {
                                this->select_on = true;
                                this->items_menu->SetItemMultiselected(this->items_menu->GetSelectedItem(), true);
                            }
                        }
                        else if(keys_down & HidNpadButton_AnyUp) {
                            if(title.title_type == cfg::TitleType::Application) {
                                const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("app_launch"), GetLanguageString("app_take_over_select") + "\n" + GetLanguageString("app_take_over_selected"), { "Yes", "Cancel" }, true);
                                if(option == 0) {
                                    UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, title.app_info.record.application_id));
                                    cfg::SaveConfig(g_Config);
                                    g_MenuApplication->ShowNotification(GetLanguageString("app_take_over_done"));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void MainMenuLayout::menu_OnSelected(const s32 prev_idx, const u32 idx) {
        this->selected_item_author_text->SetVisible(true);
        this->selected_item_version_text->SetVisible(true);
        u32 actual_idx = idx;
        if(this->homebrew_mode) {
            if(idx == 0) {
                this->selected_item_author_text->SetVisible(false);
                this->selected_item_version_text->SetVisible(false);
                this->banner_img->SetImage(cfg::GetAssetByTheme(g_Theme, "ui/BannerHomebrew.png"));
                this->selected_item_name_text->SetText(GetLanguageString("hbmenu_launch"));
            }
            else {
                actual_idx--;
                auto &hb = g_HomebrewRecordList.at(actual_idx);
                hb.EnsureControlDataLoaded();

                this->selected_item_name_text->SetText(hb.control.name);
                this->selected_item_author_text->SetText(hb.control.author);
                this->selected_item_version_text->SetText(hb.control.version);

                this->banner_img->SetImage(cfg::GetAssetByTheme(g_Theme, "ui/BannerHomebrew.png"));
            }
        }
        else {
            auto &folder = cfg::FindFolderByName(g_EntryList, this->cur_folder);
            s32 title_idx = actual_idx;
            if(this->cur_folder.empty()) {
                if(actual_idx >= g_EntryList.folders.size()) {
                    title_idx -= g_EntryList.folders.size();
                }
                else {
                    const auto &selected_folder = g_EntryList.folders.at(actual_idx);
                    this->banner_img->SetImage(cfg::GetAssetByTheme(g_Theme, "ui/BannerFolder.png"));
                    const auto folder_entry_count = selected_folder.titles.size();
                    this->selected_item_author_text->SetText(std::to_string(folder_entry_count) + " " + ((folder_entry_count == 1) ? GetLanguageString("folder_entry_single") : GetLanguageString("folder_entry_mult")));
                    this->selected_item_version_text->SetVisible(false);
                    this->selected_item_name_text->SetText(selected_folder.name);
                    title_idx = -1;
                }
            }
            if(title_idx >= 0) {
                auto &title = folder.titles.at(title_idx);
                title.EnsureControlDataLoaded();

                if(title.Is<cfg::TitleType::Application>()) {
                    this->selected_item_name_text->SetText(title.control.name + "(type " + std::to_string(title.app_info.record.type) + ", stid + " + std::to_string(title.app_info.meta_status.storageID) + ")");
                }
                else {
                    this->selected_item_name_text->SetText(title.control.name);
                }

                this->selected_item_author_text->SetText(title.control.author);
                this->selected_item_version_text->SetText(title.control.version);

                if(title.Is<cfg::TitleType::Homebrew>()) {
                    this->banner_img->SetImage(cfg::GetAssetByTheme(g_Theme, "ui/BannerHomebrew.png"));
                }
                else {
                    this->banner_img->SetImage(cfg::GetAssetByTheme(g_Theme, "ui/BannerInstalled.png"));
                }
            }
            if(!this->cur_folder.empty()) {
                // This way we know we're inside a folder
                this->banner_img->SetImage(cfg::GetAssetByTheme(g_Theme, "ui/BannerFolder.png"));
            }
        }

        if(g_MenuApplication->IsSuspended()) {
            if((prev_idx == (s32)this->items_menu->GetSuspendedItem()) && (idx != this->items_menu->GetSuspendedItem())) {
                this->mode = SuspendedImageMode::HidingLostFocus;
            }
            else if((prev_idx != (s32)this->items_menu->GetSuspendedItem()) && (idx == this->items_menu->GetSuspendedItem())) {
                this->mode = SuspendedImageMode::ShowingGainedFocus;
            }
        }
    }

    MainMenuLayout::MainMenuLayout(const u8 *captured_screen_buf, const u8 min_alpha) : last_has_connection(false), last_battery_lvl(0), last_is_charging(false), launch_fail_warn_shown(false), homebrew_mode(false), select_on(false), select_dir(false), min_alpha(min_alpha), target_alpha(0), mode(SuspendedImageMode::ShowingAfterStart), suspended_screen_alpha(0xFF) {
        const auto menu_text_x = g_MenuApplication->GetUIConfigValue<u32>("menu_folder_text_x", 30);
        const auto menu_text_y = g_MenuApplication->GetUIConfigValue<u32>("menu_folder_text_y", 200);
        const auto menu_text_size = g_MenuApplication->GetUIConfigValue<u32>("menu_folder_text_size", 25);

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

        this->banner_img = pu::ui::elm::Image::New(0, 585, cfg::GetAssetByTheme(g_Theme, "ui/BannerInstalled.png"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_image", this->banner_img);
        this->Add(this->banner_img);

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

        this->fw_text = pu::ui::elm::TextBlock::New(970, 65, g_FwVersion);
        this->fw_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "firmware_text", this->fw_text);
        this->Add(this->fw_text);

        this->guide_buttons_img = pu::ui::elm::Image::New(540, 120, cfg::GetAssetByTheme(g_Theme, "ui/GuideButtons.png"));
        g_MenuApplication->ApplyConfigForElement("main_menu", "guide_buttons", this->guide_buttons_img);
        this->Add(this->guide_buttons_img);

        this->menu_totggle_img = ClickableImage::New(520, 200, cfg::GetAssetByTheme(g_Theme, "ui/ToggleClick.png"));
        this->menu_totggle_img->SetOnClick(std::bind(&MainMenuLayout::menuToggle_Click, this));
        g_MenuApplication->ApplyConfigForElement("main_menu", "menu_toggle_button", this->menu_totggle_img);
        this->Add(this->menu_totggle_img);

        this->selected_item_name_text = pu::ui::elm::TextBlock::New(40, 610, "...");
        this->selected_item_name_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Large));
        this->selected_item_name_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_name_text", this->selected_item_name_text);
        this->Add(this->selected_item_name_text);

        this->selected_item_author_text = pu::ui::elm::TextBlock::New(45, 650, "...");
        this->selected_item_author_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->selected_item_author_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_author_text", this->selected_item_author_text);
        this->Add(this->selected_item_author_text);

        this->selected_item_version_text = pu::ui::elm::TextBlock::New(45, 675, "...");
        this->selected_item_version_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->selected_item_version_text->SetColor(g_MenuApplication->GetTextColor());
        g_MenuApplication->ApplyConfigForElement("main_menu", "banner_version_text", this->selected_item_version_text);
        this->Add(this->selected_item_version_text);

        this->items_menu = SideMenu::New(pu::ui::Color(0, 255, 120, 0xFF), cfg::GetAssetByTheme(g_Theme, "ui/Cursor.png"), cfg::GetAssetByTheme(g_Theme, "ui/Suspended.png"), cfg::GetAssetByTheme(g_Theme, "ui/Multiselect.png"), menu_text_x, menu_text_y, pu::ui::MakeDefaultFontName(menu_text_size), g_MenuApplication->GetTextColor(), 294);
        this->MoveFolder("", false);
        this->items_menu->SetOnItemSelected(std::bind(&MainMenuLayout::menu_Click, this, std::placeholders::_1, std::placeholders::_2));
        this->items_menu->SetOnSelectionChanged(std::bind(&MainMenuLayout::menu_OnSelected, this, std::placeholders::_1, std::placeholders::_2));
        g_MenuApplication->ApplyConfigForElement("main_menu", "items_menu", this->items_menu, false); // Main menu must be visible, and only Y is customizable here
        this->Add(this->items_menu);

        if(captured_screen_buf != nullptr) {
            this->Add(this->suspended_screen_img);
        }

        this->quick_menu = QuickMenu::New(cfg::GetAssetByTheme(g_Theme, "ui/QuickMenuMain.png"));
        this->Add(this->quick_menu);

        this->startup_tp = std::chrono::steady_clock::now();

        this->title_launch_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/TitleLaunch.wav"));
        this->menu_toggle_sfx = pu::audio::LoadSfx(cfg::GetAssetByTheme(g_Theme, "sound/MenuToggle.wav"));

        this->SetBackgroundImage(cfg::GetAssetByTheme(g_Theme, "ui/Background.png"));
    }

    MainMenuLayout::~MainMenuLayout() {
        pu::audio::DestroySfx(this->title_launch_sfx);
        pu::audio::DestroySfx(this->menu_toggle_sfx);
    }

    void MainMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        const auto quick_menu_on = this->quick_menu->IsOn();
        this->items_menu->SetEnabled(!quick_menu_on);
        if(quick_menu_on) {
            return;
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
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now_tp - this->startup_tp).count() >= 1000) {
            if(!this->launch_fail_warn_shown) {
                if(g_MenuApplication->LaunchFailed()) {
                    g_MenuApplication->CreateShowDialog(GetLanguageString("app_launch"), GetLanguageString("app_unexpected_error"), { GetLanguageString("ok") }, true);
                }
                this->launch_fail_warn_shown = true;
            }
        }

        if(this->suspended_screen_img) {
            switch(this->mode) {
                case SuspendedImageMode::ShowingAfterStart: {
                    if(this->items_menu->IsSuspendedItemSelected() && (this->suspended_screen_alpha <= this->min_alpha)) {
                        this->suspended_screen_alpha = this->min_alpha;
                        this->suspended_screen_img->SetAlpha(this->suspended_screen_alpha);
                        this->mode = SuspendedImageMode::Focused;
                    }
                    else if(!this->items_menu->IsSuspendedItemSelected() && (this->suspended_screen_alpha == 0)) {
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

                        UL_RC_ASSERT(menu::smi::ResumeApplication());
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
            if(!this->cur_folder.empty() && !this->homebrew_mode) {
                this->MoveFolder("", true);
            }
        }
        else if(keys_down & HidNpadButton_Plus) {
            ShowAboutDialog();
        }
        else if(keys_down & HidNpadButton_Minus) {
            this->menuToggle_Click();
        }
    }

    bool MainMenuLayout::OnHomeButtonPress() {
        if(g_MenuApplication->IsSuspended()) {
            if(this->mode == SuspendedImageMode::Focused) {
                this->mode = SuspendedImageMode::HidingForResume;
            }
        }
        else {
            this->items_menu->Rewind();
        }
        return true;
    }

    void MainMenuLayout::MoveFolder(const std::string &name, const bool fade) {
        this->items_menu->Rewind();

        if(fade) {
            g_TransitionGuard.Run([&]() {
                g_MenuApplication->FadeOut();
                this->DoMoveFolder(name);
                g_MenuApplication->FadeIn();
            });
        }
        else {
            this->DoMoveFolder(name);
        }
    }

    void MainMenuLayout::SetUser(const AccountUid user) {
        this->users_img->SetImage(acc::GetIconCacheImagePath(user));
        this->users_img->SetWidth(50);
        this->users_img->SetHeight(50);
    }

    void MainMenuLayout::menuToggle_Click() {
        pu::audio::PlaySfx(this->menu_toggle_sfx);
        this->homebrew_mode = !this->homebrew_mode;
        if(this->select_on) {
            g_MenuApplication->ShowNotification(GetLanguageString("menu_multiselect_cancel"));
            this->StopMultiselect();
        }
        this->MoveFolder("", true);
    }

    void MainMenuLayout::HandleCloseSuspended() {
        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("suspended_app"), GetLanguageString("suspended_close"), { GetLanguageString("yes"), GetLanguageString("no") }, true);
        if(option == 0) {
            this->DoTerminateApplication();
        }
    }

    void MainMenuLayout::HandleHomebrewLaunch(const cfg::TitleRecord &rec) {
        u64 title_takeover_id;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, title_takeover_id));
        const auto option = g_MenuApplication->CreateShowDialog(GetLanguageString("hb_launch"), GetLanguageString("hb_launch_conf"), { GetLanguageString("hb_applet"), GetLanguageString("hb_app"), GetLanguageString("cancel") }, true);
        if(option == 0) {
            pu::audio::PlaySfx(this->title_launch_sfx);
            
            const auto proper_ipt = CreateLaunchTargetInput(rec.hb_info.nro_target);
            UL_RC_ASSERT(menu::smi::LaunchHomebrewLibraryApplet(proper_ipt.nro_path, proper_ipt.nro_argv));

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
                    
                    const auto ipt = CreateLaunchTargetInput(rec.hb_info.nro_target);
                    const auto rc = ul::menu::smi::LaunchHomebrewApplication(ipt.nro_path, ipt.nro_argv);

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

    void MainMenuLayout::HandleMultiselectMoveToFolder(const std::string &folder) {
        if(this->select_on) {
            u32 removed_idx = 0;
            const auto root_title_count = g_EntryList.root.titles.size();
            const auto folder_count = g_EntryList.folders.size();
            for(u32 i = 0; i < root_title_count; i++) {
                auto &title = g_EntryList.root.titles.at(i - removed_idx);
                if(this->items_menu->IsItemMultiselected(folder_count + i)) {
                    if(cfg::MoveRecordTo(g_EntryList, title, folder)) {
                        removed_idx++;
                    }
                }
            }
            this->StopMultiselect();
            this->MoveFolder(this->cur_folder, true);
        }
    }

    void MainMenuLayout::StopMultiselect() {
        this->select_on = false;
        this->select_dir = false;
        this->items_menu->ResetMultiselections();
    }

    void MainMenuLayout::DoTerminateApplication() {
        this->items_menu->ResetSuspendedItem();
        g_MenuApplication->NotifyEndSuspended();
        
        if(this->suspended_screen_img) {
            this->suspended_screen_img->SetAlpha(0);
        }

        UL_RC_ASSERT(menu::smi::TerminateApplication());
    }

}