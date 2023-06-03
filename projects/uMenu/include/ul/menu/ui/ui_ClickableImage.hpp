
#pragma once
#include <pu/Plutonium>

namespace ul::menu::ui {

    class ClickableImage : public pu::ui::elm::Element {
        public:
            using OnClickCallback = std::function<void()>;

            static constexpr u64 TouchActionTimeMilliseconds = 200;

            static constexpr u8 RedColorMod = 0xC8;
            static constexpr u8 GreenColorMod = 0xC8;
            static constexpr u8 BlueColorMod = 0xFF;

        protected:
            std::string img;
            pu::sdl2::Texture img_tex;
            s32 x;
            s32 y;
            s32 w;
            s32 h;
            OnClickCallback cb;
            std::chrono::steady_clock::time_point touch_tp;
            bool touched;

        public:
            ClickableImage(const s32 x, const s32 y, const std::string &img) : Element(), img_tex(nullptr), x(x), y(y), w(0), h(0), cb(), touched(false) {
                this->SetImage(img);
            }
            PU_SMART_CTOR(ClickableImage)

            ~ClickableImage();

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

            inline std::string GetImage() {
                return this->img;
            }
            
            void SetImage(const std::string &img);
            
            inline bool IsImageValid() {
                return this->img_tex != nullptr;
            }
            
            inline void SetOnClick(OnClickCallback cb) {
                this->cb = cb;
            }

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
    };

}