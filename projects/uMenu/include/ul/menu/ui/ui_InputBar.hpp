
#pragma once
#include <pu/Plutonium>

namespace ul::menu::ui {

    class InputBar : public pu::ui::elm::Element {
        public:
            static constexpr u32 SideMargin = 20;
            static constexpr u32 InputMargin = 25;
            static constexpr u8 Alpha = 200;

        private:
            s32 x;
            s32 y;
            std::map<u64, pu::sdl2::Texture> inputs;
            pu::sdl2::Texture bg_img;

            static inline std::string GetKeyString(const u64 key) {
                std::string str;
                if(key & HidNpadButton_A) {
                    str += "\uE0A0";
                }
                if(key & HidNpadButton_B) {
                    str += "\uE0A1";
                }
                if(key & HidNpadButton_X) {
                    str += "\uE0A2";
                }
                if(key & HidNpadButton_Y) {
                    str += "\uE0A3";
                }
                if(key & HidNpadButton_L) {
                    str += "\uE0A4";
                }
                if(key & HidNpadButton_R) {
                    str += "\uE0A5";
                }
                if(key & HidNpadButton_ZL) {
                    str += "\uE0A6";
                }
                if(key & HidNpadButton_ZR) {
                    str += "\uE0A7";
                }
                if(key & HidNpadButton_StickL) {
                    str += "\uE08A";
                }
                if(key & HidNpadButton_StickR) {
                    str += "\uE08B";
                }
                if(key & HidNpadButton_Minus) {
                    str += "\uE0B6";
                }
                if(key & HidNpadButton_Plus) {
                    str += "\uE0B5";
                }
                return str;
            }

        public:
            InputBar(const s32 x, const s32 y);
            PU_SMART_CTOR(InputBar)

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

            inline s32 GetWidth() override {
                return pu::ui::render::ScreenWidth;
            }

            inline s32 GetHeight() override {
                return pu::ui::render::GetTextureHeight(this->bg_img);
            }

            void ClearInputs();
            void AddSetInput(const u64 key, const std::string &text);

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override {}
    };

}