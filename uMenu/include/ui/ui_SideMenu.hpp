
#pragma once
#include <pu/Plutonium>

namespace ui {

    class SideMenu : public pu::ui::elm::Element {
        public:
            static constexpr u32 BaseX = 98;
            static constexpr u32 ItemSize = 256;
            static constexpr u32 Margin = 20;
            static constexpr u32 ItemCount = 4;
            static constexpr u32 FocusSize = 15;
            static constexpr u32 FocusMargin = 5;
            static constexpr u32 ExtraIconSize = ItemSize + (Margin * 2);

            static constexpr u8 MoveAlphaIncrement = 30;

            static constexpr u64 ScrollBaseWaitTimeMs = 50;
            static constexpr u64 ScrollMoveWaitTimeMs = 200;

            using OnSelectCallback = std::function<void(const u64, const u32)>;
            using OnSelectionChangedCallback = std::function<void(const u32)>;

        private:
            s32 y;
            u32 selected_item_idx;
            u32 prev_selected_item_idx;
            s32 suspended_item_idx;
            u32 base_icon_idx;
            u8 move_alpha;
            u32 text_x;
            u32 text_y;
            bool enabled;
            pu::ui::Color text_clr;
            std::vector<std::string> items_icon_paths;
            std::vector<std::string> items_icon_texts;
            std::vector<bool> items_multiselected;
            OnSelectCallback on_select_cb;
            OnSelectionChangedCallback on_selection_changed_cb;
            std::vector<pu::sdl2::Texture> rendered_icons;
            std::vector<pu::sdl2::Texture> rendered_texts;
            pu::sdl2::Texture cursor_icon;
            pu::sdl2::Texture suspended_icon;
            pu::sdl2::Texture left_border_icon;
            pu::sdl2::Texture right_border_icon;
            pu::sdl2::Texture multiselect_icon;
            std::string text_font;
            std::chrono::steady_clock::time_point scroll_tp;
            bool scroll_move_flag;
            std::chrono::steady_clock::time_point scroll_move_tp;
            u32 scroll_flag;
            u32 scroll_tp_value;
            u32 scroll_count;

            inline void DoOnItemSelected(const u64 keys) {
                if(this->on_select_cb) {
                    (this->on_select_cb)(keys, this->selected_item_idx);
                }
            }

            inline void DoOnSelectionChanged() {
                if(this->on_selection_changed_cb) {
                    (this->on_selection_changed_cb)(this->selected_item_idx);
                }
            }

            inline void ClearBorderIcons() {
                pu::ui::render::DeleteTexture(this->left_border_icon);
                pu::ui::render::DeleteTexture(this->right_border_icon);
            }

            inline void ClearRenderedItems() {
                for(auto &icon_tex: this->rendered_icons) {
                    pu::ui::render::DeleteTexture(icon_tex);
                }
                this->rendered_icons.clear();

                for(auto &text_tex: this->rendered_texts) {
                    pu::ui::render::DeleteTexture(text_tex);
                }
                this->rendered_texts.clear();

                this->ClearBorderIcons();
            }

            bool IsLeftFirst();
            bool IsRightLast();
            void MoveReloadIcons(const bool moving_right);

        public:
            SideMenu(const pu::ui::Color suspended_clr, const std::string &cursor_path, const std::string &suspended_img_path, const std::string &multiselect_img_path, const s32 txt_x, const s32 txt_y, const std::string &font_name, const pu::ui::Color txt_clr, const s32 y);
            PU_SMART_CTOR(SideMenu)
            ~SideMenu();

            inline constexpr s32 GetX() override {
                return BaseX;
            }

            inline s32 GetY() override {
                return this->y;
            }

            // Note: stubbed for ApplyConfigForElement to work with this element
            inline void SetX(const s32 x) {}

            inline void SetY(const s32 y) {
                this->y = y;
            }

            inline constexpr s32 GetWidth() override {
                return pu::ui::render::ScreenWidth;
            }

            inline constexpr s32 GetHeight() override {
                return ItemSize + FocusSize + FocusMargin;
            }

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            
            inline void SetOnItemSelected(OnSelectCallback cb) {
                this->on_select_cb = cb;
            }
            
            inline void SetOnSelectionChanged(OnSelectionChangedCallback cb) {
                this->on_selection_changed_cb = cb;
            }
            
            void ClearItems();
            void AddItem(const std::string &icon, const std::string &txt = "");
            
            inline void SetSuspendedItem(const u32 idx) {
                if(idx < this->items_icon_paths.size()) {
                    this->suspended_item_idx = idx;
                }
            }

            inline void ResetSuspendedItem() {
                this->suspended_item_idx = -1;
            }

            inline void Rewind() {
                this->ClearRenderedItems();

                this->selected_item_idx = 0;
                this->scroll_count = 0;
                this->base_icon_idx = 0;
            }

            void HandleMoveLeft();
            void HandleMoveRight();

            inline u32 GetSelectedItem() {
                return this->selected_item_idx;
            }

            void UpdateBorderIcons();
            void ResetMultiselections();
            void SetItemMultiselected(const u32 idx, const bool selected);
            bool IsItemMultiselected(const u32 idx);
            bool IsAnyMultiselected();
            
            inline void SetEnabled(const bool enabled) {
                this->enabled = enabled;
            }
    };

}