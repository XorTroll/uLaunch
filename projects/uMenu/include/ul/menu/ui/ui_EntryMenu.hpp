
#pragma once
#include <ul/ent/ent_Load.hpp>
#include <pu/Plutonium>

namespace ul::menu::ui {

    class EntryMenu : public pu::ui::elm::Element {
        public:
            static constexpr u32 SideMargin = 15;
            static constexpr u32 EntryMargin = 10;
            static constexpr u32 CursorTransitionIncrement = 50;
            static constexpr u32 DefaultEntrySize = 256;
            static constexpr u32 DefaultCursorSize = 296;

        private:
            s32 y;
            s32 height;
            u32 entry_v_count;
            u32 entry_size;
            u32 base_scroll_idx;
            u32 cur_entry_idx;
            s32 cursor_transition_x;
            s32 cursor_transition_y;
            s32 cursor_transition_x_incr;
            s32 cursor_transition_y_incr;
            u32 pre_transition_entry_idx;
            pu::sdl2::Texture cursor_img;
            pu::sdl2::Texture suspended_img;
            pu::sdl2::Texture selected_img;
            std::vector<pu::sdl2::Texture> entry_imgs;
            ent::EntryPath cur_path;
            ent::Entry selected_entry;
            std::vector<ent::Entry> cur_entries;
            pu::sdl2::Texture entry_info_bg;
            bool enabled;

            void LoadUpdateEntries();
            void HandleScrollUp();
            void HandleScrollDown();

            inline bool IsCursorInTransition() {
                return (this->cursor_transition_x_incr != 0) || (this->cursor_transition_y_incr != 0);
            }

            inline void ComputeSizes(const u32 entry_h_count) {
                this->entry_size = (this->GetWidth() - 2 * SideMargin - (entry_h_count - 1) * EntryMargin) / entry_h_count;
                this->entry_v_count = (this->height - 2 * SideMargin + EntryMargin) / (this->entry_size + EntryMargin) + 1;

                const auto base_y = this->GetY() + SideMargin + pu::ui::render::GetTextureHeight(this->entry_info_bg) + SideMargin;
                while((base_y + ((this->entry_v_count > 0) ? (this->entry_v_count - 1) * this->entry_size : 0) + ((this->entry_v_count > 1) ? (this->entry_v_count - 2) * EntryMargin : 0)) >= pu::ui::render::ScreenHeight) {
                    this->entry_v_count--;
                }
            }

            inline pu::sdl2::Texture LoadEntryImage(const u32 idx) {
                if(idx >= this->cur_entries.size()) {
                    return nullptr;
                }

                const auto entry = this->cur_entries.at(idx);
                if(entry.Is<ul::ent::EntryKind::Folder>()) {
                    return pu::ui::render::LoadImage("sdmc:/umad/uitest/folder.png");
                }
                else {
                    return pu::ui::render::LoadImage(entry.metadata.icon_path);
                }
            }

        public:
            EntryMenu(const s32 y, const s32 height);
            PU_SMART_CTOR(EntryMenu)

            inline constexpr s32 GetX() override {
                return 0;
            }

            inline s32 GetY() override {
                return this->y;
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

            void OnHomeButtonPress();

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
    };

}