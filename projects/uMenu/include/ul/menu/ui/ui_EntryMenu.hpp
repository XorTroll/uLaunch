
#pragma once
#include <ul/menu/menu_Entries.hpp>
#include <ul/ul_Include.hpp>
#include <ul/util/util_Enum.hpp>
#include <pu/Plutonium>

namespace ul::menu::ui {

    class EntryMenu : public pu::ui::elm::Element {
        public:
            static constexpr u32 SideMargin = 15;
            static constexpr u32 EntryMargin = 10;
            static constexpr u32 CursorTransitionIncrement = 50;
            static constexpr u32 DefaultEntrySize = 256;
            static constexpr u32 DefaultCursorSize = 296;
            
            static constexpr s32 PseudoScrollUpIndex = -5;
            static constexpr s32 PseudoScrollDownIndex = -10;

            using FocusedEntryInputPressedCallback = std::function<void(const u64)>;
            using FocusedEntryChangedCallback = std::function<void(const bool, const bool, const bool)>;

        private:
            s32 x;
            s32 y;
            s32 height;
            u32 entry_v_count;
            u32 entry_size;
            u32 base_scroll_entry_idx;
            u32 cur_entry_idx;
            s32 cursor_transition_x;
            s32 cursor_transition_y;
            s32 cursor_transition_x_incr;
            s32 cursor_transition_y_incr;
            s32 pre_transition_base_scroll_entry_idx;
            s32 pre_transition_entry_idx;
            s32 after_transition_base_scroll_entry_idx;
            s32 after_transition_entry_idx;
            pu::sdl2::Texture cursor_img;
            pu::sdl2::Texture suspended_img;
            pu::sdl2::Texture selected_img;
            std::vector<pu::sdl2::Texture> entry_imgs;
            std::string cur_path;
            std::vector<Entry> cur_entries;
            pu::sdl2::Texture entry_info_bg;
            std::vector<bool> entries_selected;
            std::stack<u32> entry_idx_stack;
            FocusedEntryInputPressedCallback cur_entry_input_cb;
            FocusedEntryChangedCallback cur_entry_changed_cb;
            bool enabled;

            void LoadUpdateEntries();
            void HandleScrollUp();
            void HandleScrollDown();
            pu::sdl2::Texture LoadEntryImage(const u32 idx);

            void NotifyFocusedEntryChanged(const s32 prev_idx);

            inline bool IsCursorInTransition() {
                return (this->cursor_transition_x_incr != 0) || (this->cursor_transition_y_incr != 0);
            }

            inline void ComputeSizes(const u32 entry_h_count) {
                this->entry_size = (this->GetWidth() - 2 * SideMargin - (entry_h_count - 1) * EntryMargin) / entry_h_count;
                this->entry_v_count = (this->height - 2 * SideMargin + EntryMargin) / (this->entry_size + EntryMargin);

                while((this->GetY() + ((this->entry_v_count > 0) ? (this->entry_v_count - 1) * this->entry_size : 0) + ((this->entry_v_count > 1) ? (this->entry_v_count - 2) * EntryMargin : 0)) >= pu::ui::render::ScreenHeight) {
                    this->entry_v_count--;
                }
            }

        public:
            EntryMenu(const s32 x, const s32 y, const s32 height, const std::string &path, const u32 last_idx, FocusedEntryInputPressedCallback cur_entry_input_cb, FocusedEntryChangedCallback cur_entry_changed_cb);
            PU_SMART_CTOR(EntryMenu)

            void Initialize();

            inline s32 GetX() override {
                return this->x;
            }

            inline s32 GetY() override {
                return this->y;
            }

            inline void SetX(const s32 x) {
                this->x = x;
            }

            inline void SetY(const s32 y) {
                this->y = y;
            }

            inline constexpr s32 GetWidth() override {
                return pu::ui::render::ScreenWidth;
            }

            inline s32 GetHeight() override {
                return this->height;
            }

            inline void SetEnabled(const bool enabled) {
                this->enabled = enabled;
            }

            inline std::string &GetPath() {
                return this->cur_path;
            }

            inline std::vector<Entry> &GetEntries() {
                return this->cur_entries;
            }

            inline bool HasEntries() {
                return !this->cur_entries.empty();
            }

            inline bool IsInRoot() {
                return this->cur_path.length() == __builtin_strlen(MenuPath);
            }

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;

            void MoveTo(const std::string &new_path = "");

            inline u32 GetFocusedEntryIndex() {
                return this->cur_entry_idx;
            }

            inline Entry &GetFocusedEntry() {
                return this->cur_entries.at(this->cur_entry_idx);
            }

            void IncrementHorizontalCount();
            void DecrementHorizontalCount();

            void RewindEntries();

            void ResetSelection();
            void SetEntrySelected(const u32 idx, const bool selected);
            bool IsEntrySelected(const u32 idx);
            bool IsAnySelected();

            bool IsFocusedEntrySuspended();

            inline void ToggleFocusedEntrySelected() {
                this->SetEntrySelected(this->cur_entry_idx, !this->IsEntrySelected(this->cur_entry_idx));
            }

            inline void SetFocusedEntrySelected(const bool selected) {
                this->SetEntrySelected(this->cur_entry_idx, selected);
            }

            inline bool IsFocusedEntrySelected() {
                return this->IsEntrySelected(this->cur_entry_idx);
            }
    };

}