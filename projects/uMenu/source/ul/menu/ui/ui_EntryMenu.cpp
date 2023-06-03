#include <ul/menu/ui/ui_EntryMenu.hpp>
#include <ul/menu/ui/ui_Application.hpp>
#include <ul/menu/ui/ui_TransitionGuard.hpp>
#include <ul/menu/ui/ui_Util.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/cfg/cfg_Config.hpp>

extern ul::cfg::Config g_Config;
extern ul::menu::ui::Application::Ref g_Application;
extern ul::menu::ui::TransitionGuard g_TransitionGuard;
extern ul::smi::SystemStatus g_SystemStatus;

namespace ul::menu::ui {

    namespace {

        inline menu::MainMenuLayout::Ref GetMainMenuLayout() {
            return g_Application->GetLayout<menu::MainMenuLayout>();
        }

        inline bool IsAnySuspended() {
            return (g_SystemStatus.suspended_app_id > 0) || g_SystemStatus.suspended_hb_target_ipt.IsValid();
        }

        inline bool IsEntrySuspended(const ent::Entry &entry) {
            if(entry.Is<ent::EntryKind::Application>()) {
                return entry.metadata.app_metadata.app_id == g_SystemStatus.suspended_app_id;
            }
            if(entry.Is<ent::EntryKind::Homebrew>()) {
                return (entry.metadata.hb_metadata.nro_path == g_SystemStatus.suspended_hb_target_ipt.nro_path) && (entry.metadata.hb_metadata.nro_argv == g_SystemStatus.suspended_hb_target_ipt.nro_argv);
            }

            return false;
        }

        inline void ResumeCurrentApplication() {
            GetMainMenuLayout()->NotifyResume();
        }

        inline void TerminateCurrentApplication() {
            UL_RC_ASSERT(smi::TerminateApplication());
            g_SystemStatus.suspended_app_id = 0;
            g_SystemStatus.suspended_hb_target_ipt = {};
        }

    }

    void EntryMenu::LoadUpdateEntries() {
        ent::LoadEntries(this->cur_path, this->cur_entries);

        for(auto &entry_img : this->entry_imgs) {
            pu::ui::render::DeleteTexture(entry_img);
        }
        this->entry_imgs.clear();

        this->base_scroll_idx = 0;
        this->pre_transition_entry_idx = 0;
        this->cur_entry_idx = 0;
        for(u32 i = 0; i < this->entry_v_count; i++) {
            for(u32 j = 0; j < g_Config.entry_menu_h_count; j++) {
                const auto idx = j + i * g_Config.entry_menu_h_count;
                this->entry_imgs.push_back(this->LoadEntryImage(idx));
            }
        }
    }

    void EntryMenu::HandleScrollUp() {
        this->base_scroll_idx--;
        for(u32 i = 0; i < g_Config.entry_menu_h_count; i++) {
            pu::ui::render::DeleteTexture(this->entry_imgs.back());
            this->entry_imgs.pop_back();
        }
        for(u32 i = 0; i < g_Config.entry_menu_h_count; i++) {
            const auto actual_idx = (g_Config.entry_menu_h_count - 1) - i;
            this->entry_imgs.insert(this->entry_imgs.begin(), this->LoadEntryImage(this->base_scroll_idx * g_Config.entry_menu_h_count + actual_idx));
        }
    }

    void EntryMenu::HandleScrollDown() {
        const auto base_idx = this->base_scroll_idx * g_Config.entry_menu_h_count + this->entry_imgs.size();
        this->base_scroll_idx++;
        for(u32 i = 0; i < g_Config.entry_menu_h_count; i++) {
            pu::ui::render::DeleteTexture(this->entry_imgs.front());
            this->entry_imgs.erase(this->entry_imgs.begin());
        }
        for(u32 i = 0; i < g_Config.entry_menu_h_count; i++) {
            this->entry_imgs.push_back(this->LoadEntryImage(base_idx + i));
        }
    }

    EntryMenu::EntryMenu(const s32 y, const s32 height) : y(y), height(height), cur_entry_idx(0), enabled(true) {
        this->cursor_img = pu::ui::render::LoadImage("sdmc:/umad/uitest/cursor.png");
        this->suspended_img = pu::ui::render::LoadImage("sdmc:/umad/uitest/suspended.png");
        this->selected_img = pu::ui::render::LoadImage("sdmc:/umad/uitest/selected.png");
        this->entry_info_bg = pu::ui::render::LoadImage("sdmc:/umad/uitest/entry_info_bg.png");

        this->ComputeSizes(g_Config.entry_menu_h_count);
        this->cur_path = {};
        this->LoadUpdateEntries();

        this->base_scroll_idx = 0;
        this->cursor_transition_x = 0;
        this->cursor_transition_y = 0;
        this->cursor_transition_x_incr = 0;
        this->cursor_transition_y_incr = 0;
        this->selected_entry = {};
    }

    void EntryMenu::OnHomeButtonPress() {
        if(IsAnySuspended()) {
            for(u32 i = 0; i < this->entry_imgs.size(); i++) {
                const auto entry_img = this->entry_imgs.at(i);
                if(entry_img == nullptr) {
                    continue;
                }

                const auto entry = this->cur_entries.at(this->base_scroll_idx * g_Config.entry_menu_h_count + i);
                if(IsEntrySuspended(entry)) {
                    ResumeCurrentApplication();
                    break;
                }
            }
        }
    }

    void EntryMenu::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        const auto entry_info_bg_x = x + SideMargin;
        const auto entry_info_bg_y = y + SideMargin;
        drawer->RenderTexture(this->entry_info_bg, entry_info_bg_x, entry_info_bg_y);

        const auto cur_actual_entry_idx = this->IsCursorInTransition() ? this->pre_transition_entry_idx : this->cur_entry_idx;
        const auto cur_i = this->base_scroll_idx * g_Config.entry_menu_h_count + cur_actual_entry_idx;

        pu::sdl2::Texture info_main_text = nullptr;
        pu::sdl2::Texture info_sub_text_1 = nullptr;
        pu::sdl2::Texture info_sub_text_2 = nullptr;
        if(this->cur_entries.empty()) {
            info_main_text = pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge), "No entries?", { 0xFF, 0xFF, 0xFF, 0xFF });
        }
        else if(cur_i < this->cur_entries.size()) {
            const auto cur_entry = this->cur_entries.at(cur_i);

            info_main_text = pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge), cur_entry.metadata.name, { 0xFF, 0xFF, 0xFF, 0xFF });
            if(cur_entry.IsNormalEntry()) {
                info_sub_text_1 = pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small), cur_entry.metadata.author, { 0xFF, 0xFF, 0xFF, 0xFF });
                info_sub_text_2 = pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small), cur_entry.metadata.version, { 0xFF, 0xFF, 0xFF, 0xFF });
            }
        }

        const auto is_only_main_text = (info_sub_text_1 == nullptr) && (info_sub_text_2 == nullptr);

        const auto main_text_x = x + (this->GetWidth() - pu::ui::render::GetTextureWidth(info_main_text)) / 2;
        const auto main_text_y = entry_info_bg_y + (is_only_main_text ? ((pu::ui::render::GetTextureHeight(this->entry_info_bg) - pu::ui::render::GetTextureHeight(info_main_text)) / 2) : (2 * EntryMargin));
        drawer->RenderTexture(info_main_text, main_text_x, main_text_y);

        const auto sub_text_1_x = x + (this->GetWidth() - pu::ui::render::GetTextureWidth(info_sub_text_1)) / 2;
        const auto sub_text_1_y = main_text_y + pu::ui::GetDefaultFontSize(pu::ui::DefaultFontSize::MediumLarge);
        drawer->RenderTexture(info_sub_text_1, sub_text_1_x, sub_text_1_y);

        const auto sub_text_2_x = x + (this->GetWidth() - pu::ui::render::GetTextureWidth(info_sub_text_2)) / 2;
        const auto sub_text_2_y = sub_text_1_y + pu::ui::GetDefaultFontSize(pu::ui::DefaultFontSize::Small);
        drawer->RenderTexture(info_sub_text_2, sub_text_2_x, sub_text_2_y);

        pu::ui::render::DeleteTexture(info_main_text);
        pu::ui::render::DeleteTexture(info_sub_text_1);
        pu::ui::render::DeleteTexture(info_sub_text_2);

        if(this->cur_entries.empty()) {
            return;
        }

        const auto base_y = entry_info_bg_y + pu::ui::render::GetTextureHeight(this->entry_info_bg) + SideMargin;

        s32 suspended_x = 0;
        s32 suspended_y = 0;
        s32 selected_x = 0;
        s32 selected_y = 0;
        for(u32 i = 0; i < this->entry_imgs.size(); i++) {
            const auto entry_img = this->entry_imgs.at(i);
            if(entry_img == nullptr) {
                continue;
            }

            const auto idx_x = i % g_Config.entry_menu_h_count;
            const auto idx_y = i / g_Config.entry_menu_h_count;
            const auto entry_x = x + SideMargin + idx_x * (this->entry_size + EntryMargin);
            const auto entry_y = base_y + SideMargin + idx_y * (this->entry_size + EntryMargin);
            drawer->RenderTexture(entry_img, entry_x, entry_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(this->entry_size, this->entry_size));

            const auto entry = this->cur_entries.at(this->base_scroll_idx * g_Config.entry_menu_h_count + i);
            if(IsEntrySuspended(entry)) {
                suspended_x = entry_x;
                suspended_y = entry_y;
            }
            if(this->selected_entry.IsValid() && (this->selected_entry == entry)) {
                selected_x = entry_x;
                selected_y = entry_y;
            }
        }

        const auto cur_actual_entry_idx_x = cur_actual_entry_idx % g_Config.entry_menu_h_count;
        const auto cur_actual_entry_idx_y = cur_actual_entry_idx / g_Config.entry_menu_h_count;

        const auto cursor_size = (u32)(((double)this->entry_size / (double)DefaultEntrySize) * DefaultCursorSize);
        const auto cursor_x = x + SideMargin + cur_actual_entry_idx_x * (this->entry_size + EntryMargin) - ((cursor_size - this->entry_size) / 2) + (this->IsCursorInTransition() ? this->cursor_transition_x : 0);
        const auto cursor_y = base_y + SideMargin + cur_actual_entry_idx_y * (this->entry_size + EntryMargin) - ((cursor_size - this->entry_size) / 2) + (this->IsCursorInTransition() ? this->cursor_transition_y : 0);

        const auto cursor_width = std::min(cursor_size, this->GetWidth() - cursor_x);
        drawer->RenderTexture(this->cursor_img, cursor_x, cursor_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(cursor_width, cursor_size));

        if((suspended_x > x) && (suspended_y > y)) {
            const auto susp_x = suspended_x - ((cursor_size - this->entry_size) / 2);
            const auto susp_y = suspended_y - ((cursor_size - this->entry_size) / 2);

            drawer->RenderTexture(this->suspended_img, susp_x, susp_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(cursor_size, cursor_size));
        }

        if((selected_x > x) && (selected_y > y)) {
            const auto sel_x = selected_x - ((cursor_size - this->entry_size) / 2);
            const auto sel_y = selected_y - ((cursor_size - this->entry_size) / 2);

            drawer->RenderTexture(this->selected_img, sel_x, sel_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(cursor_size, cursor_size));
        }

        if(this->IsCursorInTransition()) {
            this->cursor_transition_x += this->cursor_transition_x_incr;
            this->cursor_transition_y += this->cursor_transition_y_incr;
            const auto abs_x = static_cast<u32>((this->cursor_transition_x > 0) ? this->cursor_transition_x : -this->cursor_transition_x);
            const auto abs_y = static_cast<u32>((this->cursor_transition_y > 0) ? this->cursor_transition_y : -this->cursor_transition_y);
            if((abs_x >= (SideMargin + this->entry_size)) || (abs_y >= (SideMargin + this->entry_size))) {
                this->cursor_transition_x = 0;
                this->cursor_transition_y = 0;
                this->cursor_transition_x_incr = 0;
                this->cursor_transition_y_incr = 0;
            }
        }
    }

    void EntryMenu::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->IsCursorInTransition()) {
            return;
        }
        if(!this->enabled) {
            return;
        }

        auto status = menu::InputStatus::Normal;
        const auto cur_i = this->base_scroll_idx * g_Config.entry_menu_h_count + this->cur_entry_idx;
        const auto is_cur_entry_valid = cur_i < this->cur_entries.size();
        if(cur_i < this->cur_entries.size()) {
            const auto cur_entry = this->cur_entries.at(cur_i);
            if(IsEntrySuspended(cur_entry)) {
                status = status | menu::InputStatus::CurrentEntrySuspended;
            }
            else if(cur_entry.Is<ent::EntryKind::Folder>()) {
                status = status | menu::InputStatus::CurrentEntryFolder;
            }
        }
        else {
            status = status | menu::InputStatus::CurrentEntryInvalid;
        }
        if(this->selected_entry.IsValid()) {
            status = status | menu::InputStatus::EntrySelected;
        }
        if(!this->cur_path.IsRoot()) {
            status = status | menu::InputStatus::InFolder;
        }
        GetMainMenuLayout()->UpdateInputIfDifferent(status);

        if((keys_down & HidNpadButton_Up) || (keys_held & HidNpadButton_StickLUp)) {
            if(!this->cur_entries.empty()) {
                const auto idx_y = this->cur_entry_idx / g_Config.entry_menu_h_count;
                if(idx_y > 0) {
                    this->pre_transition_entry_idx = this->cur_entry_idx;
                    this->cur_entry_idx -= g_Config.entry_menu_h_count;
                    this->cursor_transition_y_incr = -CursorTransitionIncrement;
                }
                else if(this->base_scroll_idx > 0) {
                    this->HandleScrollUp();
                }
            }
        }
        else if((keys_down & HidNpadButton_Down) || (keys_held & HidNpadButton_StickLDown)) {
            if(!this->cur_entries.empty()) {
                const auto idx_y = this->cur_entry_idx / g_Config.entry_menu_h_count;
                if((idx_y + 1) < this->entry_v_count) {
                    this->pre_transition_entry_idx = this->cur_entry_idx;
                    this->cur_entry_idx += g_Config.entry_menu_h_count;
                    this->cursor_transition_y_incr = CursorTransitionIncrement;
                }
                else if(((this->base_scroll_idx + 1) * g_Config.entry_menu_h_count) < this->cur_entries.size()) {
                    this->HandleScrollDown();
                }
            }
        }
        else if((keys_down & HidNpadButton_Left) || (keys_held & HidNpadButton_StickLLeft)) {
            if(!this->cur_entries.empty()) {
                if(this->cur_entry_idx > 0) {
                    this->pre_transition_entry_idx = this->cur_entry_idx;
                    this->cur_entry_idx--;
                    this->cursor_transition_x_incr = -CursorTransitionIncrement;
                }
                else if(this->base_scroll_idx > 0) {
                    this->pre_transition_entry_idx = this->cur_entry_idx;
                    this->cur_entry_idx += (g_Config.entry_menu_h_count - 1);
                    this->cursor_transition_x_incr = -CursorTransitionIncrement;
                    this->HandleScrollUp();
                }
            }
        }
        else if((keys_down & HidNpadButton_Right) || (keys_held & HidNpadButton_StickLRight)) {
            if(!this->cur_entries.empty()) {
                if((this->cur_entry_idx + 1) < this->entry_imgs.size()) {
                    this->pre_transition_entry_idx = this->cur_entry_idx;
                    this->cur_entry_idx++;
                    this->cursor_transition_x_incr = CursorTransitionIncrement;
                }
                else if(((this->base_scroll_idx + 1) * g_Config.entry_menu_h_count) < this->cur_entries.size()) {
                    this->pre_transition_entry_idx = this->cur_entry_idx;
                    this->cur_entry_idx -= (g_Config.entry_menu_h_count - 1);
                    this->cursor_transition_x_incr = CursorTransitionIncrement;
                    this->HandleScrollDown();
                }
            }
        }
        else if(keys_down & HidNpadButton_A) {
            if(is_cur_entry_valid) {
                const auto cur_entry = this->cur_entries.at(cur_i);
                if(IsEntrySuspended(cur_entry)) {
                    ResumeCurrentApplication();
                }
                else {
                    if(cur_entry.Is<ent::EntryKind::Application>()) {
                        auto do_launch = true;
                        if(IsAnySuspended()) {
                            const auto opt = g_Application->CreateShowDialog("uMad", "Would you like to close the currently opened content before launching this content?", { "Yes", "Cancel" }, true);
                            if(opt == 0) {
                                TerminateCurrentApplication();
                            }
                            else {
                                do_launch = false;
                            }
                        }

                        if(do_launch) {
                            UL_RC_ASSERT(accountInitialize(AccountServiceType_System));
                            AccountUid users[ACC_USER_LIST_SIZE];
                            s32 count;
                            UL_RC_ASSERT(accountListAllUsers(users, ACC_USER_LIST_SIZE, &count));
                            accountExit();
                            UL_RC_ASSERT(smi::SetSelectedUser(users[0]));

                            UL_RC_ASSERT(smi::LaunchApplication(cur_entry.metadata.app_metadata.app_id));
                            g_Application->CloseWithFadeOut();
                        }
                    }
                    else if(cur_entry.Is<ent::EntryKind::Homebrew>()) {
                        auto do_launch = true;
                        auto launch_as_app = false;

                        const auto opt = g_Application->CreateShowDialog("Homebrew launch", "How would you like to launch this homebrew entry?", { "Applet", "Application", "Cancel" }, true);
                        switch(opt) {
                            case 0: {
                                break;
                            }
                            case 1: {
                                if(g_Config.hb_application_takeover_program_id == 0) {
                                    g_Application->ShowNotification("No homebrew takeover application is set. Please select an application via its entry options.");
                                    do_launch = false;
                                }
                                else {
                                    launch_as_app = true;
                                }
                                break;
                            }
                            default: {
                                do_launch = false;
                                break;
                            }
                        }

                        if(do_launch) {
                            if(launch_as_app) {
                                if(IsAnySuspended()) {
                                    const auto opt = g_Application->CreateShowDialog("uMad", "Would you like to close the currently opened content before launching this content?", { "Yes", "Cancel" }, true);
                                    if(opt == 0) {
                                        TerminateCurrentApplication();
                                    }
                                    else {
                                        do_launch = false;
                                    }
                                }

                                if(do_launch) {
                                    UL_RC_ASSERT(accountInitialize(AccountServiceType_System));
                                    AccountUid users[ACC_USER_LIST_SIZE];
                                    s32 count;
                                    UL_RC_ASSERT(accountListAllUsers(users, ACC_USER_LIST_SIZE, &count));
                                    accountExit();
                                    UL_RC_ASSERT(smi::SetSelectedUser(users[0]));

                                    UL_RC_ASSERT(smi::LaunchHomebrewApplication(cur_entry.metadata.hb_metadata.nro_path, cur_entry.metadata.hb_metadata.nro_argv));
                                }
                            }
                            else {
                                UL_RC_ASSERT(smi::LaunchHomebrewLibraryApplet(cur_entry.metadata.hb_metadata.nro_path, cur_entry.metadata.hb_metadata.nro_argv));
                            }
                            
                            if(do_launch) {
                                g_Application->CloseWithFadeOut();
                            }
                        }
                    }
                    else if(cur_entry.Is<ent::EntryKind::Folder>()) {
                        g_Application->FadeOut();
                        this->cur_path.Push(cur_entry.metadata.name);
                        this->LoadUpdateEntries();
                        g_Application->FadeIn();
                    }
                }
            }
        }
        else if(keys_down & HidNpadButton_B) {
            if(!this->cur_path.IsRoot()) {
                g_Application->FadeOut();
                this->cur_path.Pop();
                this->LoadUpdateEntries();
                g_Application->FadeIn();
            }
        }
        else if(keys_down & HidNpadButton_X) {
            if(this->selected_entry.IsValid()) {
                this->selected_entry = {};
            }
            else if(is_cur_entry_valid) {
                const auto cur_entry = this->cur_entries.at(cur_i);
                if(IsEntrySuspended(cur_entry)) {
                    const auto opt = g_Application->CreateShowDialog("uMad", "Would you like to close the currently opened content?", { "Yes", "Cancel" }, true);
                    if(opt == 0) {
                        TerminateCurrentApplication();
                    }
                }
            }
        }
        else if(keys_down & HidNpadButton_Y) {
            if(this->selected_entry.IsValid()) {
                g_Application->FadeOut();
                if(this->cur_path == this->selected_entry.path) {
                    // Change index
                    this->selected_entry.Move(cur_i);
                }
                else {
                    // Move to folder
                    this->selected_entry.UpdatePath(this->cur_path);
                }
                this->selected_entry = {};
                this->LoadUpdateEntries();
                g_Application->FadeIn();
            }
            else if(is_cur_entry_valid) {
                this->selected_entry = this->cur_entries.at(cur_i);
            }
        }
        else if(keys_down & HidNpadButton_StickL) {
            if(is_cur_entry_valid) {
                std::string msg = "Entry information:\n\n";
                std::vector<std::string> options;
                auto cur_entry = this->cur_entries.at(cur_i);
                switch(cur_entry.kind) {
                    case ent::EntryKind::Application: {
                        msg += "Name: " + cur_entry.metadata.name + "\n";
                        msg += "Author: " + cur_entry.metadata.author + "\n";
                        msg += "Version: " + cur_entry.metadata.version + "\n";
                        msg += "\n";
                        msg += "Application ID: " + util::FormatProgramId(cur_entry.metadata.app_metadata.app_id) + "\n";

                        if(g_Config.hb_application_takeover_program_id == cur_entry.metadata.app_metadata.app_id) {
                            msg += "This entry is the current homebrew takeover application.";
                        }
                        else if(g_Config.hb_application_takeover_program_id != 0) {
                            msg += "This entry isn't the current homebrew takeover application.";
                        }
                        else {
                            msg += "There is no current homebrew takeover application set.";
                        }
                        msg += "\n";
                        options.push_back("Set as homebrew takeover application");
                        break;
                    }
                    case ent::EntryKind::Homebrew: {
                        msg += "Name: " + cur_entry.metadata.name + "\n";
                        msg += "Author: " + cur_entry.metadata.author + "\n";
                        msg += "Version: " + cur_entry.metadata.version + "\n";
                        msg += "\n";
                        msg += "Homebrew path: '" + cur_entry.metadata.hb_metadata.nro_path + "'\n";
                        msg += "Homebrew arguments: '" + cur_entry.metadata.hb_metadata.nro_argv + "'\n";

                        // TODO: edit argv via swkbd?
                        options.push_back("Edit arguments");
                        break;
                    }
                    case ent::EntryKind::Folder: {
                        msg += "Folder name: " + cur_entry.metadata.name + "\n";

                        options.push_back("Remove folder");
                        break;
                    }
                    default:
                        break;
                }

                options.push_back("Cancel");
                const auto opt = g_Application->CreateShowDialog("Entry options", msg, options, true);

                switch(cur_entry.kind) {
                    case ent::EntryKind::Application: {
                        switch(opt) {
                            case 0: {
                                g_Config.hb_application_takeover_program_id = cur_entry.metadata.app_metadata.app_id;
                                if(R_SUCCEEDED(smi::SetHomebrewTakeoverApplication(g_Config.hb_application_takeover_program_id))) {
                                    g_Application->ShowNotification("The homebrew takeover application was updated.");
                                }
                                else {
                                    g_Application->ShowNotification("Unable to update the homebrew takeover application...");
                                }
                                break;
                            }
                        }
                        break;
                    }
                    case ent::EntryKind::Homebrew: {
                        switch(opt) {
                            case 0: {
                                SwkbdConfig swkbd;
                                UL_RC_ASSERT(swkbdCreate(&swkbd, 0));

                                char new_argv[2048] = {};
                                swkbdConfigSetStringLenMax(&swkbd, sizeof(new_argv) - 1);
                                swkbdConfigSetInitialText(&swkbd, cur_entry.metadata.hb_metadata.nro_argv.c_str());

                                const auto rc = swkbdShow(&swkbd, new_argv, sizeof(new_argv));
                                if(R_SUCCEEDED(rc)) {
                                    cur_entry.metadata.hb_metadata.nro_argv.assign(reinterpret_cast<const char*>(new_argv));
                                    // TODO: implement entry saving
                                    g_Application->ShowNotification("The homebrew's launch arguments were updated.");
                                }
                                break;
                            }
                        }
                        break;
                    }
                    case ent::EntryKind::Folder: {
                        switch(opt) {
                            case 0: {
                                const auto opt_2 = g_Application->CreateShowDialog("Folder remove", "Are you sure you want to remove this folder?\nThe entries inside will be moved to the parent folder", { "Yes", "Cancel" }, true);
                                if(opt_2 == 0) {
                                    g_Application->FadeOut();
                                    const auto folder_path = this->cur_path + cur_entry.metadata.name;

                                    std::vector<ent::Entry> folder_entries;
                                    ent::LoadEntries(folder_path, folder_entries);

                                    for(auto &entry: folder_entries) {
                                        entry.UpdatePath(this->cur_path);
                                    }

                                    cur_entry.Remove();
                                    this->LoadUpdateEntries();
                                    g_Application->FadeIn();
                                }
                                break;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }

                
            }
        }
        else if(keys_down & HidNpadButton_StickR) {
            const auto opt = g_Application->CreateShowDialog("uMad", "Would you like to create a new folder?", { "Yes", "Cancel" }, true);
            if(opt == 0) {
                SwkbdConfig cfg;
                UL_RC_ASSERT(swkbdCreate(&cfg, 0));

                swkbdConfigSetGuideText(&cfg, "Folder name");

                char out_name[500] = {};
                const auto rc = swkbdShow(&cfg, out_name, sizeof(out_name));
                swkbdClose(&cfg);
                if(R_SUCCEEDED(rc)) {
                    ent::EnsureCreatePath(this->cur_path + out_name);

                    g_Application->FadeOut();
                    this->LoadUpdateEntries();
                    g_Application->FadeIn();
                }
            }
        }
        else if(keys_down & HidNpadButton_Minus) {
            if(g_Config.entry_menu_h_count > 1) {
                g_Config.entry_menu_h_count--;
                this->ComputeSizes(g_Config.entry_menu_h_count);

                const auto new_count = g_Config.entry_menu_h_count * this->entry_v_count;
                if(new_count == 0) {
                    g_Config.entry_menu_h_count++;
                    this->ComputeSizes(g_Config.entry_menu_h_count);
                }
                else {
                    g_Application->FadeOut();
                    g_Config.Save();
                    this->LoadUpdateEntries();
                    g_Application->FadeIn();
                }
            }
        }
        else if(keys_down & HidNpadButton_Plus) {
            g_Config.entry_menu_h_count++;
            this->ComputeSizes(g_Config.entry_menu_h_count);

            g_Application->FadeOut();
            g_Config.Save();
            this->LoadUpdateEntries();
            g_Application->FadeIn();
        }
        else if((keys_down & HidNpadButton_L) || (keys_down & HidNpadButton_R)) {
            ShowHelpDialog();
        }
        else if((keys_down & HidNpadButton_ZL) || (keys_down & HidNpadButton_ZR)) {
            GetMainMenuLayout()->ToggleQuickMenu();
        }
    }

}