#include <ul/menu/ui/ui_EntryMenu.hpp>
#include <ul/menu/ui/ui_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/util/util_String.hpp>

extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;
extern ul::menu::ui::TransitionGuard g_TransitionGuard;
extern ul::cfg::Config g_Config;
extern ul::cfg::Theme g_Theme;

namespace ul::menu::ui {

    namespace {

        u32 g_MenuEntryHorizontalCount = 0;

        void SetHorizontalEntryCount(const u32 count) {
            UL_ASSERT_TRUE(g_Config.SetEntry(cfg::ConfigEntryId::MenuEntryHorizontalCount, static_cast<u64>(count)));
            cfg::SaveConfig(g_Config);

            g_MenuEntryHorizontalCount = count;
        }

    }

    void EntryMenu::LoadUpdateEntries() {
        for(auto &entry_img : this->entry_imgs) {
            pu::ui::render::DeleteTexture(entry_img);
        }
        this->entry_imgs.clear();
        this->ResetSelection();

        this->base_scroll_entry_idx = 0;
        this->cur_entry_idx = 0;
        if(!this->entry_idx_stack.empty()) {
            const auto idx = this->entry_idx_stack.top();
            if(idx < this->cur_entries.size()) {
                this->base_scroll_entry_idx = (idx / g_MenuEntryHorizontalCount) * g_MenuEntryHorizontalCount;
                this->cur_entry_idx = idx;
            }
            this->entry_idx_stack.pop();
        }

        for(u32 i = 0; i < this->entry_v_count; i++) {
            for(u32 j = 0; j < g_MenuEntryHorizontalCount; j++) {
                const auto idx = this->base_scroll_entry_idx + j + i * g_MenuEntryHorizontalCount;
                if(idx < this->cur_entries.size()) {
                    this->entry_imgs.push_back(this->LoadEntryImage(idx));
                }
            }
        }
    }

    void EntryMenu::HandleScrollUp() {
        this->base_scroll_entry_idx -= g_MenuEntryHorizontalCount;
        const auto expected_size = g_MenuEntryHorizontalCount * (this->entry_v_count - 1);
        while(this->entry_imgs.size() > expected_size) {
            pu::ui::render::DeleteTexture(this->entry_imgs.back());
            this->entry_imgs.pop_back();
        }
        for(u32 i = 0; i < g_MenuEntryHorizontalCount; i++) {
            const auto actual_idx = (g_MenuEntryHorizontalCount - 1) - i;
            const auto entry_i = this->base_scroll_entry_idx + actual_idx;
            if(entry_i < this->cur_entries.size()) {
                this->entry_imgs.insert(this->entry_imgs.begin(), this->LoadEntryImage(entry_i));
            }
        }
    }

    void EntryMenu::HandleScrollDown() {
        const auto orig_base_scroll_entry_idx = this->base_scroll_entry_idx;
        this->base_scroll_entry_idx += g_MenuEntryHorizontalCount * this->entry_v_count;
        while(this->base_scroll_entry_idx >= this->cur_entries.size()) {
            this->base_scroll_entry_idx -= g_MenuEntryHorizontalCount;
        }
        const auto move_count = this->base_scroll_entry_idx - orig_base_scroll_entry_idx;

        for(u32 i = 0; i < move_count; i++) {
            if(!this->entry_imgs.empty()) {
                pu::ui::render::DeleteTexture(this->entry_imgs.front());
                this->entry_imgs.erase(this->entry_imgs.begin());
            }
        }
        for(u32 i = 0; i < move_count; i++) {
            const auto entry_i = this->base_scroll_entry_idx + i;
            if(entry_i < this->cur_entries.size()) {
                this->entry_imgs.push_back(this->LoadEntryImage(entry_i));
            }
        }
    }

    pu::sdl2::Texture EntryMenu::LoadEntryImage(const u32 idx) {
        const auto &entry = this->cur_entries.at(idx);
        auto icon_path = entry.control.icon_path;
        if(icon_path.empty()) {
            if(entry.Is<EntryType::Folder>()) {
                icon_path = cfg::GetAssetByTheme(g_Theme, "ui/Folder.png");
            }
            else if(entry.Is<EntryType::Application>()) {
                icon_path = cfg::GetAssetByTheme(g_Theme, "ui/DefaultApplicationIcon.png");
            }
            else if(entry.Is<EntryType::Homebrew>()) {
                icon_path = cfg::GetAssetByTheme(g_Theme, "ui/DefaultHomebrewIcon.png");
            }
        }
        
        return pu::ui::render::LoadImage(icon_path);
    }

    void EntryMenu::NotifyFocusedEntryChanged(const s32 prev_idx) {
        const auto has_prev = (prev_idx >= 0) && (prev_idx < (s32)this->cur_entries.size());
        const auto prev_suspended = has_prev ? g_MenuApplication->IsEntrySuspended(this->cur_entries.at(prev_idx)) : false;
        const auto cur_suspended = g_MenuApplication->IsEntrySuspended(this->GetFocusedEntry());
        this->cur_entry_changed_cb(has_prev, prev_suspended, cur_suspended);
    }

    EntryMenu::EntryMenu(const s32 x, const s32 y, const s32 height, const std::string &path, const u32 last_idx, FocusedEntryInputPressedCallback cur_entry_input_cb, FocusedEntryChangedCallback cur_entry_changed_cb) : x(x), y(y), height(height), cur_entry_idx(0), entries_selected(), entry_idx_stack(), cur_entry_input_cb(cur_entry_input_cb), cur_entry_changed_cb(cur_entry_changed_cb), enabled(true) {
        this->cursor_img = pu::ui::render::LoadImage(cfg::GetAssetByTheme(g_Theme, "ui/Cursor.png"));
        this->suspended_img = pu::ui::render::LoadImage(cfg::GetAssetByTheme(g_Theme, "ui/Suspended.png"));
        this->selected_img = pu::ui::render::LoadImage(cfg::GetAssetByTheme(g_Theme, "ui/Selected.png"));

        u64 menu_h_count;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::MenuEntryHorizontalCount, menu_h_count));
        g_MenuEntryHorizontalCount = menu_h_count;

        this->ComputeSizes(g_MenuEntryHorizontalCount);
        this->cur_path = path;
        this->entry_idx_stack.push(last_idx);

        this->base_scroll_entry_idx = 0;
        this->cursor_transition_x = 0;
        this->cursor_transition_y = 0;
        this->cursor_transition_x_incr = 0;
        this->cursor_transition_y_incr = 0;
        this->after_transition_base_scroll_entry_idx = -1;
        this->after_transition_entry_idx = -1;
    }

    void EntryMenu::Initialize() {
        this->MoveTo("");
        this->NotifyFocusedEntryChanged(-1);
    }

    void EntryMenu::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        if(!this->cur_entries.empty()) {
            const auto cur_actual_entry_idx = this->IsCursorInTransition() ? this->pre_transition_entry_idx : this->cur_entry_idx;
            const auto base_y = y;

            const auto cursor_size = (u32)(((double)this->entry_size / (double)DefaultEntrySize) * DefaultCursorSize);

            for(u32 i = 0; i < this->entry_imgs.size(); i++) {
                const auto entry_img = this->entry_imgs.at(i);

                const auto idx_x = i % g_MenuEntryHorizontalCount;
                const auto idx_y = i / g_MenuEntryHorizontalCount;
                const auto entry_x = x + SideMargin + idx_x * (this->entry_size + EntryMargin);
                const auto entry_y = base_y + SideMargin + idx_y * (this->entry_size + EntryMargin);
                drawer->RenderTexture(entry_img, entry_x, entry_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(this->entry_size, this->entry_size));

                const auto entry_i = this->base_scroll_entry_idx + i;
                const auto entry = this->cur_entries.at(entry_i);

                if(g_MenuApplication->IsEntrySuspended(entry)) {
                    const auto susp_x = entry_x - ((cursor_size - this->entry_size) / 2);
                    const auto susp_y = entry_y - ((cursor_size - this->entry_size) / 2);

                    drawer->RenderTexture(this->suspended_img, susp_x, susp_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(cursor_size, cursor_size));
                }

                if(this->IsEntrySelected(entry_i)) {
                    const auto sel_x = entry_x - ((cursor_size - this->entry_size) / 2);
                    const auto sel_y = entry_y - ((cursor_size - this->entry_size) / 2);

                    drawer->RenderTexture(this->selected_img, sel_x, sel_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(cursor_size, cursor_size));
                }
            }

            const auto cur_actual_entry_idx_x = (cur_actual_entry_idx - this->base_scroll_entry_idx) % g_MenuEntryHorizontalCount;
            const auto cur_actual_entry_idx_y = (cur_actual_entry_idx - this->base_scroll_entry_idx) / g_MenuEntryHorizontalCount;

            const auto cursor_x = x + SideMargin + cur_actual_entry_idx_x * (this->entry_size + EntryMargin) - ((cursor_size - this->entry_size) / 2) + (this->IsCursorInTransition() ? this->cursor_transition_x : 0);
            const auto cursor_y = base_y + SideMargin + cur_actual_entry_idx_y * (this->entry_size + EntryMargin) - ((cursor_size - this->entry_size) / 2) + (this->IsCursorInTransition() ? this->cursor_transition_y : 0);

            drawer->RenderTexture(this->cursor_img, cursor_x, cursor_y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(cursor_size, cursor_size));
        }

        if(this->IsCursorInTransition()) {
            this->cursor_transition_x += this->cursor_transition_x_incr;
            this->cursor_transition_y += this->cursor_transition_y_incr;
            const u32 abs_x = abs(this->cursor_transition_x);
            const u32 abs_y = abs(this->cursor_transition_y);
            if((abs_x >= (SideMargin + this->entry_size)) || (abs_y >= (SideMargin + this->entry_size))) {
                this->cursor_transition_x = 0;
                this->cursor_transition_y = 0;
                this->cursor_transition_x_incr = 0;
                this->cursor_transition_y_incr = 0;

                if(this->after_transition_base_scroll_entry_idx >= 0) {
                    this->base_scroll_entry_idx = this->after_transition_base_scroll_entry_idx;
                    this->NotifyFocusedEntryChanged(this->pre_transition_entry_idx);
                }
                else if(this->after_transition_base_scroll_entry_idx == PseudoScrollUpIndex) {
                    const auto prev_cur_i = this->cur_entry_idx;
                    this->HandleScrollUp();
                    this->NotifyFocusedEntryChanged(prev_cur_i);
                }
                else if(this->after_transition_base_scroll_entry_idx == PseudoScrollDownIndex) {
                    const auto prev_cur_i = this->cur_entry_idx;
                    this->HandleScrollDown();
                    this->NotifyFocusedEntryChanged(prev_cur_i);
                }

                if(this->after_transition_entry_idx >= 0) {
                    this->cur_entry_idx = this->after_transition_entry_idx;
                    this->NotifyFocusedEntryChanged(this->pre_transition_entry_idx);
                }

                this->after_transition_base_scroll_entry_idx = -1;
                this->after_transition_entry_idx = -1;
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

        const auto prev_cur_i = this->cur_entry_idx;
        if((keys_down & HidNpadButton_Up) || (keys_held & HidNpadButton_StickLUp)) {
            if(!this->cur_entries.empty()) {
                if(this->cur_entry_idx >= g_MenuEntryHorizontalCount) {
                    if((this->base_scroll_entry_idx <= this->cur_entry_idx) && ((this->base_scroll_entry_idx + g_MenuEntryHorizontalCount) > this->cur_entry_idx)) {
                        this->cur_entry_idx -= g_MenuEntryHorizontalCount;
                        this->HandleScrollUp();
                        this->NotifyFocusedEntryChanged(prev_cur_i);
                    }
                    else {
                        this->pre_transition_entry_idx = this->cur_entry_idx;
                        this->after_transition_entry_idx = this->cur_entry_idx - g_MenuEntryHorizontalCount;
                        this->cursor_transition_y_incr = -CursorTransitionIncrement;
                        this->NotifyFocusedEntryChanged(prev_cur_i);
                    }
                }
            }
        }
        else if((keys_down & HidNpadButton_Down) || (keys_held & HidNpadButton_StickLDown)) {
            if(!this->cur_entries.empty()) {
                if((this->cur_entry_idx + g_MenuEntryHorizontalCount) < this->cur_entries.size()) {
                    if((this->base_scroll_entry_idx + g_MenuEntryHorizontalCount * this->entry_v_count) <= (this->cur_entry_idx + g_MenuEntryHorizontalCount)) {
                        this->cur_entry_idx += g_MenuEntryHorizontalCount;
                        this->HandleScrollDown();
                        this->NotifyFocusedEntryChanged(this->pre_transition_entry_idx);
                    }
                    else {
                        this->pre_transition_entry_idx = this->cur_entry_idx;
                        this->after_transition_entry_idx = this->cur_entry_idx + g_MenuEntryHorizontalCount;
                        this->cursor_transition_y_incr = CursorTransitionIncrement;
                        this->NotifyFocusedEntryChanged(this->pre_transition_entry_idx);
                    }
                }
            }
        }
        else if((keys_down & HidNpadButton_Left) || (keys_held & HidNpadButton_StickLLeft)) {
            if(!this->cur_entries.empty()) {
                if(this->cur_entry_idx > 0) {
                    if((this->base_scroll_entry_idx > 0) && (this->base_scroll_entry_idx == this->cur_entry_idx)) {
                        this->pre_transition_entry_idx = this->cur_entry_idx;
                        this->after_transition_entry_idx = this->cur_entry_idx - 1;
                        this->cursor_transition_x_incr = -CursorTransitionIncrement;
                        this->after_transition_base_scroll_entry_idx = PseudoScrollUpIndex;
                    }
                    else {
                        this->pre_transition_entry_idx = this->cur_entry_idx;
                        this->after_transition_entry_idx = this->cur_entry_idx - 1;
                        this->cursor_transition_x_incr = -CursorTransitionIncrement;
                        this->NotifyFocusedEntryChanged(prev_cur_i);
                    }
                    
                }
            }
        }
        else if((keys_down & HidNpadButton_Right) || (keys_held & HidNpadButton_StickLRight)) {
            if(!this->cur_entries.empty()) {
                if((this->cur_entry_idx + 1) < this->cur_entries.size()) {
                    if(((this->cur_entry_idx - this->base_scroll_entry_idx) == (this->entry_v_count * g_MenuEntryHorizontalCount - 1))) {
                        this->pre_transition_entry_idx = this->cur_entry_idx;
                        this->after_transition_entry_idx = this->cur_entry_idx + 1;
                        this->cursor_transition_x_incr = CursorTransitionIncrement;
                        this->after_transition_base_scroll_entry_idx = PseudoScrollDownIndex;
                    }
                    else {
                        this->pre_transition_entry_idx = this->cur_entry_idx;
                        this->after_transition_entry_idx = this->cur_entry_idx + 1;
                        this->cursor_transition_x_incr = CursorTransitionIncrement;
                        this->NotifyFocusedEntryChanged(prev_cur_i);
                    }
                }
            }
        }
        else {
            this->cur_entry_input_cb(keys_down);
        }
    }

    void EntryMenu::MoveTo(const std::string &new_path) {
        const auto reloading = new_path.empty();
        const auto going_back = new_path.length() < this->cur_path.length();
        if(!reloading) {
            this->cur_path = new_path;
        }

        const auto old_entry_idx = this->cur_entry_idx;
        
        this->cur_entries = LoadEntries(this->cur_path);
        this->LoadUpdateEntries();

        if(!reloading && !going_back) {
            this->entry_idx_stack.push(old_entry_idx);
        }
    }

    void EntryMenu::IncrementHorizontalCount() {
        auto h_count = g_MenuEntryHorizontalCount;
        h_count++;

        g_MenuApplication->FadeOut();
        if(!this->entry_idx_stack.empty()) {
            this->entry_idx_stack.top() = 0;
        }
        SetHorizontalEntryCount(h_count);
        this->ComputeSizes(h_count);
        this->LoadUpdateEntries();
        g_MenuApplication->FadeIn();
    }
    
    void EntryMenu::DecrementHorizontalCount() {
        auto h_count = g_MenuEntryHorizontalCount;
        if(h_count > 1) {
            h_count--;

            g_MenuApplication->FadeOut();
            this->ComputeSizes(h_count);

            const auto new_count = h_count * this->entry_v_count;
            if(new_count == 0) {
                h_count++;
                this->ComputeSizes(h_count);
            }
            else {
                
                if(!this->entry_idx_stack.empty()) {
                    this->entry_idx_stack.top() = 0;
                }
                SetHorizontalEntryCount(h_count);
                this->LoadUpdateEntries();
            }
            g_MenuApplication->FadeIn();
        }
    }

    void EntryMenu::RewindEntries() {
        this->base_scroll_entry_idx = 0;
        this->cur_entry_idx = 0;
        this->LoadUpdateEntries();
    }

    void EntryMenu::ResetSelection() {
        this->entries_selected.clear();
        for(u32 i = 0; i < this->cur_entries.size(); i++) {
            this->entries_selected.push_back(false);
        }
    }

    void EntryMenu::SetEntrySelected(const u32 idx, const bool selected) {
        if(idx < this->entries_selected.size()) {
            this->entries_selected.at(idx) = selected;
        }
    }

    bool EntryMenu::IsEntrySelected(const u32 idx) {
        if(idx < this->entries_selected.size()) {
            return this->entries_selected.at(idx);
        }

        return false;
    }

    bool EntryMenu::IsAnySelected() {
        for(const auto &selected: this->entries_selected) {
            if(selected) {
                return true;
            }
        }

        return false;
    }

    bool EntryMenu::IsFocusedEntrySuspended() {
        if(this->cur_entries.empty()) {
            return false;
        }
        else {
            return g_MenuApplication->IsEntrySuspended(this->GetFocusedEntry());
        }
    }

}