
#pragma once
#include <ul/ent/ent_Load.hpp>
#include <pu/Plutonium>

namespace ul::menu::ui {

    class InputBar : public pu::ui::elm::Element {
        public:
            static constexpr u32 SideMargin = 20;
            static constexpr u32 InputMargin = 25;
            static constexpr u8 Alpha = 200;

        private:
            s32 y;
            std::map<u64, pu::sdl2::Texture> inputs;
            pu::sdl2::Texture bg_img;

            static inline std::string GetKeyString(const u64 key) {
                std::string str;
                if(key & HidNpadButton_A) {
                    str += "A"; //"\uE0A0";
                    str += "/";
                }
                if(key & HidNpadButton_B) {
                    str += "B"; //"\uE0A1";
                    str += "/";
                }
                if(key & HidNpadButton_X) {
                    str += "X"; //"\uE0A2";
                    str += "/";
                }
                if(key & HidNpadButton_Y) {
                    str += "Y"; //"\uE0A3";
                    str += "/";
                }
                if(key & HidNpadButton_L) {
                    str += "L"; //"\uE0A4";
                    str += "/";
                }
                if(key & HidNpadButton_R) {
                    str += "R"; //"\uE0A5";
                    str += "/";
                }
                if(key & HidNpadButton_ZL) {
                    str += "ZL"; //"\uE0A6";
                    str += "/";
                }
                if(key & HidNpadButton_ZR) {
                    str += "ZR"; //"\uE0A7";
                    str += "/";
                }
                if(key & HidNpadButton_StickL) {
                    str += "LS"; //"\uE08A";
                    str += "/";
                }
                if(key & HidNpadButton_StickR) {
                    str += "RS"; //"\uE08B";
                    str += "/";
                }
                if(key & HidNpadButton_Minus) {
                    str += "-"; //"\uE0B6";
                    str += "/";
                }
                if(key & HidNpadButton_Plus) {
                    str += "+"; //"\uE0B5";
                    str += "/";
                }
                if(!str.empty()) {
                    str.pop_back();
                }
                return str;
            }

        public:
            InputBar(const s32 y);
            PU_SMART_CTOR(InputBar)

            inline constexpr s32 GetX() override {
                return 0;
            }

            inline s32 GetY() override {
                return this->y;
            }

            inline s32 GetWidth() override {
                return 1280;
            }

            inline s32 GetHeight() override {
                return pu::ui::render::ScreenHeight - this->GetY();
            }

            void ClearInputs();
            void AddSetInput(const u64 key, const std::string &text);

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override {}
    };

}