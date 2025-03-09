
#pragma once
#include <pu/Plutonium>

namespace ul::menu::ui {

    class InputBar : public pu::ui::elm::Element {
        public:
            static constexpr u32 SideMargin = 30;
            static constexpr u32 InputMargin = 38;
            static constexpr u8 Alpha = 200;
            static constexpr u32 MetaDpadNpadButton = HidNpadButton_Left | HidNpadButton_Right | HidNpadButton_Up | HidNpadButton_Down;
            static constexpr u32 MetaAnyStickNpadButton = HidNpadButton_StickL | HidNpadButton_StickR;
            static constexpr u32 MetaHomeNpadButton = HidNpadButton_Verification; // Random unused bit for us

        private:
            s32 x;
            s32 y;
            std::map<u64, pu::sdl2::Texture> inputs;
            pu::sdl2::Texture bar_bg;

            static inline std::string GetKeyString(const u64 key) {
                std::string str;

                // Order is intentional

                if(key & MetaHomeNpadButton) {
                    str += "\uE0F4 ";
                }

                if(key & MetaDpadNpadButton) {
                    str += "\uE0EA ";
                }
                if(key & MetaAnyStickNpadButton) {
                    str += "\uE100 ";
                }

                if(key & HidNpadButton_A) {
                    str += "\uE0E0 ";
                }
                if(key & HidNpadButton_B) {
                    str += "\uE0E1 ";
                }
                if(key & HidNpadButton_X) {
                    str += "\uE0E2 ";
                }
                if(key & HidNpadButton_Y) {
                    str += "\uE0E3 ";
                }

                if(key & HidNpadButton_L) {
                    str += "\uE0E4 ";
                }
                if(key & HidNpadButton_R) {
                    str += "\uE0E5 ";
                }
                if(key & HidNpadButton_ZL) {
                    str += "\uE0E6 ";
                }
                if(key & HidNpadButton_ZR) {
                    str += "\uE0E7 ";
                }

                if(key & HidNpadButton_Plus) {
                    str += "\uE0F1 ";
                }
                if(key & HidNpadButton_Minus) {
                    str += "\uE0F2 ";
                }
                return str;
            }

        public:
            InputBar(const s32 x, const s32 y, const std::string &bg_path);
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
                return pu::ui::render::GetTextureWidth(this->bar_bg);
            }

            inline s32 GetHeight() override {
                return pu::ui::render::GetTextureHeight(this->bar_bg);
            }

            void ClearInputs();
            void AddSetInput(const u64 key, const std::string &text);

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override {}
    };

}
