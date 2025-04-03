
#pragma once
#include <ul/menu/menu_Entries.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Enum.hpp>
#include <pu/Plutonium>

namespace ul::menu::ui {

    class EntryMenu : public pu::ui::elm::Element {
        public:
            enum class SwipeMode {
                None,
                LeftSingle,
                RightSingle,
                LeftPage,
                RightPage,
                Rewind
            };

            static constexpr u32 HorizontalSideMargin = 60;
            static constexpr u32 VerticalSideMargin = 35;
            static constexpr u32 EntryVerticalMargin = 32;
            static constexpr u32 CursorTransitionIncrementSteps = 6;
            static constexpr u32 DefaultEntrySize = 256;
            static constexpr u32 MinScalableEntrySize = DefaultEntrySize * 1.5; // Equivalent to keeping 256x256 in 1280x720 resolution (otherwise they would be too small in 1080p)
            static constexpr u32 DefaultOverTextSideMargin = 15;
            static constexpr u32 DefaultProgressBarHeight = 25;
            static constexpr pu::ui::Color ProgressBackgroundColor = { 0, 0, 0, 215 };
            static constexpr pu::ui::Color ProgressForegroundColor = { 0, 210, 210, 215 };
            static constexpr u8 MoveOverIconAlpha = 220;
            static constexpr u32 DefaultMoveOverIconOffset = 8;
            static constexpr u32 EntryShowIncrementSteps = 4;
            static constexpr u32 EntriesSwipePageIncrementSteps = 20;
            static constexpr u32 EntriesSwipeSingleIncrementSteps = 6;
            static constexpr u32 EntriesSwipeRewindIncrementSteps = 36;

            using FocusedEntryInputPressedCallback = std::function<void(const u64)>;
            using FocusedEntryChangedCallback = std::function<void(const bool, const bool, const bool)>;
            using FocusedEntryChangeStartedCallback = std::function<void()>;

        private:
            s32 x;
            s32 y;
            u32 entry_width_count;
            u32 entry_size;
            u32 base_entry_idx_x;
            u32 cur_entry_idx;
            s32 cursor_transition_x;
            pu::ui::SigmoidIncrementer<s32> cursor_transition_x_incr;
            s32 cursor_transition_y;
            pu::ui::SigmoidIncrementer<s32> cursor_transition_y_incr;
            s32 pre_transition_base_entry_idx_x;
            s32 pre_transition_entry_idx;
            s32 after_transition_base_entry_idx_x;
            s32 after_transition_entry_idx;
            pu::sdl2::Texture cursor_over_icon;
            u32 cursor_size;
            pu::sdl2::Texture border_over_icon;
            u32 border_size;
            pu::sdl2::Texture suspended_over_icon;
            u32 suspended_size;
            pu::sdl2::Texture selected_over_icon;
            u32 selected_size;
            pu::sdl2::Texture hb_takeover_app_over_icon;
            u32 hb_takeover_app_size;
            pu::sdl2::Texture gamecard_over_icon;
            u32 gamecard_size;
            pu::sdl2::Texture corrupted_over_icon;
            u32 corrupted_size;
            pu::sdl2::Texture not_launchable_over_icon;
            u32 not_launchable_size;
            pu::sdl2::Texture needs_update_over_icon;
            u32 needs_update_size;
            std::vector<pu::sdl2::TextureHandle::Ref> entry_icons;
            std::vector<pu::sdl2::Texture> entry_over_texts;
            std::vector<float> entry_progresses;
            std::vector<u8> load_img_entry_alphas;
            std::vector<pu::ui::SigmoidIncrementer<u8>> load_img_entry_incrs;
            std::string cur_path;
            std::vector<Entry> cur_entries;
            pu::sdl2::Texture entry_info_bg;
            std::stack<u32> entry_idx_stack;
            FocusedEntryInputPressedCallback cur_entry_input_cb;
            FocusedEntryChangedCallback cur_entry_changed_cb;
            FocusedEntryChangeStartedCallback cur_entry_change_started_cb;
            bool enabled;
            s32 selected_entry_idx;
            pu::sdl2::TextureHandle::Ref empty_entry_icon;
            pu::sdl2::TextureHandle::Ref folder_entry_icon;
            pu::sdl2::TextureHandle::Ref selected_entry_icon;
            u32 entry_h_margin;
            u32 v_side_margin;
            std::vector<Entry> entries_to_add;
            std::vector<Entry> entries_to_remove;
            u32 pending_load_img_entry_start_idx;
            u32 pending_load_img_entry_ext_idx;
            bool pending_load_img_done;
            u32 entry_page_count;
            u32 entry_total_count;
            SwipeMode swipe_mode;
            u32 entries_base_swipe_neg_offset;
            pu::ui::SigmoidIncrementer<u32> entries_base_swipe_neg_offset_incr;
            u32 extra_entry_swipe_show_h_count;

            inline bool IsNotInSwipe() {
                return this->swipe_mode == SwipeMode::None;
            }

            inline bool IsNotInLargeSwipe() {
                return (this->swipe_mode != SwipeMode::LeftPage) && (this->swipe_mode != SwipeMode::RightPage) && (this->swipe_mode != SwipeMode::Rewind);
            }

            void SwipeSingleLeft();
            void SwipeSingleRight();
            void SwipeToPreviousPage();
            void SwipeToNextPage();
            void SwipeRewind();

            pu::sdl2::TextureHandle::Ref LoadEntryIconTexture(const Entry &entry);
            void CleanOverTexts();
            void LoadOverText(const u32 idx);
            bool LoadEntry(const u32 idx);

            void NotifyFocusedEntryChanged(const s32 prev_entry_idx, const s32 is_prev_entry_suspended_override = -1);

            inline bool IsCursorInTransition() {
                return !this->cursor_transition_x_incr.IsDone() || !this->cursor_transition_y_incr.IsDone();
            }

            inline void ComputeSizes(const u32 entry_height_count) {
                this->entry_size = (u32)((double)(this->GetHeight() - 2 * VerticalSideMargin - (entry_height_count - 1) * EntryVerticalMargin) / (double)entry_height_count);
                // Let's not overscale icons, avoid them looking too ugly
                if(this->entry_size > MinScalableEntrySize) {
                    this->entry_size = MinScalableEntrySize;
                }
                this->v_side_margin = (u32)((double)(this->GetHeight() - entry_height_count * this->entry_size - (entry_height_count - 1) * EntryVerticalMargin) / 2.0f);

                this->entry_width_count = (u32)((double)(this->GetWidth() - 2 * HorizontalSideMargin + EntryVerticalMargin) / (double)(this->entry_size + EntryVerticalMargin));
                this->entry_h_margin = (u32)((double)(this->GetWidth() - 2 * HorizontalSideMargin - this->entry_width_count * this->entry_size) / (double)(this->entry_width_count - 1));

                this->entry_page_count = this->entry_width_count * entry_height_count;
                this->entry_total_count = ((this->cur_entries.size() / this->entry_page_count) + 1) * this->entry_page_count;

                #define _COMPUTE_OVER_SIZE(name) { \
                    const auto over_def_size = pu::ui::render::GetTextureWidth(this->name##_over_icon); \
                    this->name##_size = (u32)((double)this->entry_size * ((double)over_def_size / (double)MinScalableEntrySize)); \
                }

                _COMPUTE_OVER_SIZE(cursor)
                _COMPUTE_OVER_SIZE(border)
                _COMPUTE_OVER_SIZE(suspended)
                _COMPUTE_OVER_SIZE(selected)
                _COMPUTE_OVER_SIZE(hb_takeover_app)
                _COMPUTE_OVER_SIZE(gamecard)
                _COMPUTE_OVER_SIZE(corrupted)
                _COMPUTE_OVER_SIZE(not_launchable)
                _COMPUTE_OVER_SIZE(needs_update)
            }

            void SetFocusedEntryIndex(const u32 idx);
            Entry &GetEntry(const s32 index);

            void OrganizeEntries();

        public:
            EntryMenu(const s32 x, const s32 y, const std::string &path, FocusedEntryInputPressedCallback cur_entry_input_cb, FocusedEntryChangedCallback cur_entry_changed_cb, FocusedEntryChangeStartedCallback cur_entry_change_started_cb);
            PU_SMART_CTOR(EntryMenu)

            void Initialize(const u32 last_idx, const std::string &new_path = "");

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
                return pu::ui::render::ScreenWidth - this->x;
            }

            inline s32 GetHeight() override {
                return pu::ui::render::ScreenHeight - this->y;
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

            bool IsInRoot();

            inline bool IsMenuStart() {
                return this->base_entry_idx_x == 0;
            }

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;

            void MoveTo(const std::string &new_path = "", const bool force_pop_idx = false);

            void OrganizeUpdateEntries(const s32 prev_entry_suspended_override = -1);
            void NotifyEntryAdded(const Entry &entry);
            void NotifyEntryRemoved(const Entry &entry);

            inline u32 GetFocusedEntryIndex() {
                return this->cur_entry_idx;
            }

            bool IsFocusedNonemptyEntry();

            inline Entry &GetFocusedEntry() {
                return this->GetEntry(this->cur_entry_idx);
            }

            void UpdateEntryProgress(const u32 idx, const float progress = NAN);

            inline bool IsFocusedEntryInProgress() {
                return !std::isnan(this->entry_progresses.at(this->cur_entry_idx));
            }

            void IncrementEntryHeightCount();
            bool CanDecrementEntryHeightCount();
            void DecrementEntryHeightCount();

            bool MoveToPreviousPage();
            bool MoveToNextPage();
            void Rewind();

            void ResetSelection();
            void SetEntrySelected(const u32 idx, const bool selected);
            bool IsEntrySelected(const u32 idx);
            bool IsAnySelected();

            inline Entry &GetSelectedEntry() {
                return this->GetEntry(this->selected_entry_idx);
            }

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
