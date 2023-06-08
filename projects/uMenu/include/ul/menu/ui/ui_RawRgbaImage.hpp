
#pragma once
#include <pu/Plutonium>

namespace ul::menu::ui {

    class RawRgbaImage : public pu::ui::elm::Element {
        private:
            s32 x;
            s32 y;
            s32 w;
            s32 h;
            pu::sdl2::Texture img_tex;
            u8 alpha;
        
        public:
            RawRgbaImage(const s32 x, const s32 y, const u8 *rgba_data, const s32 w, const s32 h, const u32 pix_num);
            PU_SMART_CTOR(RawRgbaImage)
            ~RawRgbaImage();

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

            inline void SetAlpha(const u8 alpha) {
                this->alpha = alpha;
            }
            
            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override {}
    };

}