#include <ul/menu/ui/ui_SideMenu.hpp>

namespace ul::menu::ui {

    bool SideMenu::IsLeftFirst() {
        auto base_x = GetProcessedX();
        constexpr auto first_item_x = BaseX;
        for(u32 i = 0; i < this->rendered_icons.size(); i++) {
            if((base_x == first_item_x) && (this->selected_item_idx == (this->base_icon_idx + i))) {
                return true;
            }
            base_x += ItemSize + Margin;
        }

        return false;
    }
    
    bool SideMenu::IsRightLast() {
        if(this->selected_item_idx == (this->items_icon_paths.size() - 1)) {
            return true;
        }

        auto base_x = GetProcessedX();
        constexpr auto last_item_x = BaseX + (Margin + ItemSize) * (ItemCount - 1);
        for(u32 i = 0; i < this->rendered_icons.size(); i++) {
            if((base_x == last_item_x) && (this->selected_item_idx == (this->base_icon_idx + i))) {
                return true;
            }
            base_x += ItemSize + Margin;
        }

        return false;
    }

    void SideMenu::MoveReloadIcons(const bool moving_right) {
        if(moving_right) {
            auto icon_tex = pu::ui::render::LoadImage(this->items_icon_paths.at(this->selected_item_idx));
            this->rendered_icons.push_back(icon_tex);
            
            auto text_tex = pu::ui::render::RenderText(this->text_font, this->items_icon_texts.at(this->selected_item_idx), this->text_clr);
            this->rendered_texts.push_back(text_tex);

            if(this->rendered_icons.size() > ItemCount) {
                pu::ui::render::DeleteTexture(this->rendered_icons.front());
                this->rendered_icons.erase(this->rendered_icons.begin());
                pu::ui::render::DeleteTexture(this->rendered_texts.front());
                this->rendered_texts.erase(this->rendered_texts.begin());
                this->base_icon_idx++;
            }
        }
        else {
            auto icon_tex = pu::ui::render::LoadImage(this->items_icon_paths.at(this->selected_item_idx));
            this->rendered_icons.insert(this->rendered_icons.begin(), icon_tex);
            const auto text = this->items_icon_texts.at(this->selected_item_idx);
            pu::sdl2::Texture text_tex = nullptr;
            if(!text.empty()) {
                text_tex = pu::ui::render::RenderText(this->text_font, text, this->text_clr);
            }
            this->rendered_texts.insert(this->rendered_texts.begin(), text_tex);

            this->base_icon_idx--;
            if(this->rendered_icons.size() > ItemCount) {
                pu::ui::render::DeleteTexture(this->rendered_icons.back());
                this->rendered_icons.pop_back();
                pu::ui::render::DeleteTexture(this->rendered_texts.back());
                this->rendered_texts.pop_back();
            }
        }
        this->UpdateBorderIcons();
    }

    SideMenu::SideMenu(const pu::ui::Color suspended_clr, const std::string &cursor_path, const std::string &suspended_img_path, const std::string &multiselect_img_path, const s32 txt_x, const s32 txt_y, const std::string &font_name, const pu::ui::Color txt_clr, const s32 y) : selected_item_idx(0), prev_selected_item_idx(-1), suspended_item_idx(-1), base_icon_idx(0), move_alpha(0), text_x(txt_x), text_y(txt_y), enabled(true), text_clr(txt_clr), on_select_cb(), on_selection_changed_cb(), left_border_icon(nullptr), right_border_icon(nullptr), text_font(font_name), scroll_flag(0), scroll_tp_value(50), scroll_count(0) {
        this->cursor_icon = pu::ui::render::LoadImage(cursor_path);
        this->suspended_icon = pu::ui::render::LoadImage(suspended_img_path);
        this->multiselect_icon = pu::ui::render::LoadImage(multiselect_img_path);
        this->SetY(y);
    }

    SideMenu::~SideMenu() {
        pu::ui::render::DeleteTexture(this->cursor_icon);
        pu::ui::render::DeleteTexture(this->suspended_icon);
        this->ClearItems();
    }

    void SideMenu::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        if(this->items_icon_paths.empty()) {
            return;
        }

        if(this->rendered_icons.empty()) {
            for(u32 i = 0; i < std::min(static_cast<size_t>(ItemCount), this->items_icon_paths.size() - this->base_icon_idx); i++) {
                auto icon_tex = pu::ui::render::LoadImage(this->items_icon_paths.at(this->base_icon_idx + i));
                const auto text = this->items_icon_texts.at(this->base_icon_idx + i);
                this->rendered_icons.push_back(icon_tex);

                pu::sdl2::Texture text_tex = nullptr;
                if(!text.empty()) {
                    text_tex = pu::ui::render::RenderText(this->text_font, text, this->text_clr);
                }
                this->rendered_texts.push_back(text_tex);
            }
            this->UpdateBorderIcons();
            this->DoOnSelectionChanged();
        }

        auto base_x = x;
        for(u32 i = 0; i < this->rendered_icons.size(); i++) {
            auto icon_tex = this->rendered_icons.at(i);
            drawer->RenderTexture(icon_tex, base_x, y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(ItemSize, ItemSize));
            
            auto text_tex = this->rendered_texts.at(i);
            if(text_tex != nullptr) {
                drawer->RenderTexture(text_tex, base_x + this->text_x, y + this->text_y);
            }
            if(this->IsItemMultiselected(this->base_icon_idx + i)) {
                drawer->RenderTexture(this->multiselect_icon, base_x - Margin, y - Margin, pu::ui::render::TextureRenderOptions::WithCustomDimensions(ExtraIconSize, ExtraIconSize));
            }
            if(this->suspended_item_idx >= 0) {
                if((this->base_icon_idx + i) == static_cast<u32>(this->suspended_item_idx)) {
                    if(this->suspended_icon != nullptr) {
                        drawer->RenderTexture(this->suspended_icon, base_x - Margin, y - Margin, pu::ui::render::TextureRenderOptions::WithCustomDimensions(ExtraIconSize, ExtraIconSize));
                    }
                }
            }
            if(this->cursor_icon != nullptr) {
                if((this->base_icon_idx + i) == this->selected_item_idx) {
                    drawer->RenderTexture(this->cursor_icon, base_x - Margin, y - Margin, pu::ui::render::TextureRenderOptions::WithCustomAlphaAndDimensions(0xFF - this->move_alpha, ExtraIconSize, ExtraIconSize));
                }
                else if((s32)(this->base_icon_idx + i) == this->prev_selected_item_idx) {
                    drawer->RenderTexture(this->cursor_icon, base_x - Margin, y - Margin, pu::ui::render::TextureRenderOptions::WithCustomAlphaAndDimensions(this->move_alpha, ExtraIconSize, ExtraIconSize));
                }
            }
            base_x += ItemSize + Margin;
        }

        if(this->left_border_icon != nullptr) {
            drawer->RenderTexture(this->left_border_icon, x - ItemSize - Margin, y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(ItemSize, ItemSize));
        }
        if(this->right_border_icon != nullptr) {
            drawer->RenderTexture(this->right_border_icon, x + ((ItemSize + Margin) * ItemCount), y, pu::ui::render::TextureRenderOptions::WithCustomDimensions(ItemSize, ItemSize));
        }

        if(move_alpha > 0) {
            s32 tmp_alpha = move_alpha - MoveAlphaIncrement;
            if(tmp_alpha < 0) {
                tmp_alpha = 0;
            }
            move_alpha = static_cast<u8>(tmp_alpha);
        }
    }

    void SideMenu::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        if(this->rendered_icons.empty()) {
            return;
        }
        if(!this->enabled) {
            return;
        }

        if(keys_down & HidNpadButton_AnyLeft) {
            HandleMoveLeft();
        }
        else if(keys_down & HidNpadButton_AnyRight) {
            HandleMoveRight();
        }
        else if(!touch_pos.IsEmpty()) {
            auto base_x = this->GetProcessedX();
            const auto y = this->GetProcessedY();
            if(this->cursor_icon != nullptr) {
                for(u32 i = 0; i < this->rendered_icons.size(); i++) {
                    constexpr auto item_size = static_cast<s32>(ItemSize);
                    if(touch_pos.HitsRegion(base_x, y, item_size, item_size)) {
                        if((this->base_icon_idx + i) == this->selected_item_idx) {
                            this->DoOnItemSelected(HidNpadButton_A);
                        }
                        else {
                            this->prev_selected_item_idx = this->selected_item_idx;
                            this->selected_item_idx = this->base_icon_idx + i;
                            this->move_alpha = 0xFF;
                            this->DoOnSelectionChanged();
                        }
                        break;
                    }
                    base_x += ItemSize + Margin;
                }
            }
        }
        else {
            this->DoOnItemSelected(keys_down);
        }

        if(keys_held & HidNpadButton_AnyLeft) {
            if(this->scroll_flag == 1) {
                const auto cur_tp = std::chrono::steady_clock::now();
                const u64 diff = std::chrono::duration_cast<std::chrono::milliseconds>(cur_tp - this->scroll_tp).count();
                if(diff >= ScrollMoveWaitTimeMs) {
                    if(this->scroll_move_flag) {
                        const u64 move_diff = std::chrono::duration_cast<std::chrono::milliseconds>(cur_tp - this->scroll_move_tp).count();
                        if(move_diff >= this->scroll_tp_value) {
                            if(this->scroll_count > ItemCount) {
                                this->scroll_count = 0;
                                this->scroll_tp_value /= 2;
                            }
                            this->scroll_move_flag = false;
                            this->HandleMoveLeft();
                            this->scroll_count++;
                        }
                    }
                    else {
                        this->scroll_move_tp = std::chrono::steady_clock::now();
                        this->scroll_move_flag = true;
                    }
                }
            }
            else {
                this->scroll_flag = 1;
                this->scroll_tp = std::chrono::steady_clock::now();
            }
        }
        else if(keys_held & HidNpadButton_AnyRight) {
            if(this->scroll_flag == 2) {
                const auto cur_tp = std::chrono::steady_clock::now();
                const u64 diff = std::chrono::duration_cast<std::chrono::milliseconds>(cur_tp - this->scroll_tp).count();
                if(diff >= ScrollMoveWaitTimeMs) {
                    if(this->scroll_move_flag) {
                        const u64 move_diff = std::chrono::duration_cast<std::chrono::milliseconds>(cur_tp - this->scroll_move_tp).count();
                        if(move_diff >= this->scroll_tp_value) {
                            if(this->scroll_count > ItemCount) {
                                this->scroll_count = 0;
                                this->scroll_tp_value /= 2;
                            }
                            this->scroll_move_flag = false;
                            this->HandleMoveRight();
                            this->scroll_count++;
                        }
                    }
                    else {
                        this->scroll_move_tp = std::chrono::steady_clock::now();
                        this->scroll_move_flag = true;
                    }
                }
            }
            else {
                this->scroll_flag = 2;
                this->scroll_tp = std::chrono::steady_clock::now();
            }
        }
        else {
            this->scroll_flag = 0;
            this->scroll_count = 0;
            this->scroll_tp_value = ScrollBaseWaitTimeMs;
        }
    }

    void SideMenu::ClearItems() {
        this->ClearRenderedItems();

        this->items_icon_paths.clear();
        this->items_icon_texts.clear();
        this->items_multiselected.clear();

        this->selected_item_idx = 0;
        this->base_icon_idx = 0;
        this->suspended_item_idx = -1;
    }

    void SideMenu::AddItem(const std::string &icon, const std::string &txt) {
        this->items_icon_paths.push_back(icon);
        this->items_icon_texts.push_back(txt);
        this->items_multiselected.push_back(false);
    }

    void SideMenu::HandleMoveLeft() {
        if(this->selected_item_idx > 0) {
            const auto is_left_first = IsLeftFirst();
            this->prev_selected_item_idx = this->selected_item_idx;
            this->selected_item_idx--;
            if(is_left_first) {
                MoveReloadIcons(false);
            }
            else {
                this->move_alpha = 0xFF;
            }

            this->DoOnSelectionChanged();
        }
    }

    void SideMenu::HandleMoveRight() {
        if((this->selected_item_idx + 1) < this->items_icon_paths.size()) {
            const auto is_right_last = IsRightLast();
            this->prev_selected_item_idx = this->selected_item_idx;
            this->selected_item_idx++;
            if(is_right_last) {
                MoveReloadIcons(true);
            }
            else {
                this->move_alpha = 0xFF;
            }

            this->DoOnSelectionChanged();
        }
    }

    void SideMenu::UpdateBorderIcons() {
        this->ClearBorderIcons();

        if(this->base_icon_idx > 0) {
            this->left_border_icon = pu::ui::render::LoadImage(this->items_icon_paths.at(this->base_icon_idx - 1));
        }
        if((this->base_icon_idx + ItemCount) < this->items_icon_paths.size()) {
            this->right_border_icon = pu::ui::render::LoadImage(this->items_icon_paths.at(this->base_icon_idx + ItemCount));
        }
    }

    void SideMenu::ResetMultiselections() {
        this->items_multiselected.clear();
        for(u32 i = 0; i < this->items_icon_paths.size(); i++) {
            this->items_multiselected.push_back(false);
        }
    }

    void SideMenu::SetItemMultiselected(const u32 idx, const bool selected) {
        if(idx < this->items_multiselected.size()) {
            this->items_multiselected.at(idx) = selected;
        }
    }

    bool SideMenu::IsItemMultiselected(const u32 idx) {
        if(idx < this->items_multiselected.size()) {
            return this->items_multiselected.at(idx);
        }

        return false;
    }

    bool SideMenu::IsAnyMultiselected() {
        for(const auto &multiselected: this->items_multiselected) {
            if(multiselected) {
                return true;
            }
        }

        return false;
    }

}