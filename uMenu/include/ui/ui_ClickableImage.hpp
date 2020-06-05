
// Grabbed from Goldleaf's source code

#pragma once
#include <ul_Include.hpp>
#include <pu/Plutonium>

namespace ui {

    class ClickableImage : public pu::ui::elm::Element {

        private:
            inline void Dispose() {
                if(this->ntex != nullptr) {
                    pu::ui::render::DeleteTexture(this->ntex);
                    this->ntex = nullptr;
                }
            }

        protected:
            std::string img;
            pu::sdl2::Texture ntex;
            s32 x;
            s32 y;
            s32 w;
            s32 h;
            std::function<void()> cb;
            std::chrono::steady_clock::time_point touchtp;
            bool touched;

        public:
            ClickableImage(s32 x, s32 y, const std::string &img) : Element(), ntex(nullptr), x(x), y(y), w(0), h(0), cb([&](){}), touched(false) {
                this->SetImage(img);
            }

            ~ClickableImage() {
                this->Dispose();
            }

            PU_SMART_CTOR(ClickableImage)

            s32 GetX();
            void SetX(s32 x);
            s32 GetY();
            void SetY(s32 y);
            s32 GetWidth();
            void SetWidth(s32 w);
            s32 GetHeight();
            void SetHeight(s32 h);
            std::string GetImage();
            void SetImage(const std::string &img);
            bool IsImageValid();
            void SetOnClick(std::function<void()> cb);
            void OnRender(pu::ui::render::Renderer::Ref &drawer, s32 x, s32 y);
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos);

    };

}