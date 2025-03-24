
#pragma once
#include <pu/Plutonium>

namespace ul::menu::ui {

    class MultiTextBlock : public pu::ui::elm::Element {
        private:
            s32 x;
            s32 y;
            s32 w;
            s32 h;
            std::vector<pu::ui::elm::TextBlock::Ref> text_blocks;

        public:
            MultiTextBlock(const s32 x, const s32 y) : Element(), x(x), y(y), w(0), h(0), text_blocks() {}
            PU_SMART_CTOR(MultiTextBlock)

            inline s32 GetX() override {
                return this->x;
            }

            inline void SetX(const s32 x) {
                this->x = x;
            }

            inline s32 GetY() override {
                return this->y;
            }

            inline void SetY(const s32 y) {
                this->y = y;
            }

            inline s32 GetWidth() override {
                return this->w;
            }

            inline void SetWidth(const s32 w) {
                this->w = w;
            }

            inline s32 GetHeight() override {
                return this->h;
            }

            inline void SetHeight(const s32 h) {
                this->h = h;
            }

            void UpdatePositionsSizes();

            void Add(pu::ui::elm::TextBlock::Ref text);
            void Clear();

            inline pu::ui::elm::TextBlock::Ref &Get(const u32 index) {
                return this->text_blocks.at(index);
            }

            inline std::vector<pu::ui::elm::TextBlock::Ref> &GetAll() {
                return this->text_blocks;
            }

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
    };

}
