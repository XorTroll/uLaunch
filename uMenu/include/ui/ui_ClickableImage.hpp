
// Grabbed from Goldleaf's source code

#pragma once
#include <ul_Include.hpp>
#include <pu/Plutonium>

namespace ui
{
    class ClickableImage : public pu::ui::elm::Element
    {
        public:
            ClickableImage(s32 X, s32 Y, pu::String Image);
            PU_SMART_CTOR(ClickableImage)
            ~ClickableImage();

            s32 GetX();
            void SetX(s32 X);
            s32 GetY();
            void SetY(s32 Y);
            s32 GetWidth();
            void SetWidth(s32 Width);
            s32 GetHeight();
            void SetHeight(s32 Height);
            pu::String GetImage();
            void SetImage(pu::String Image);
            bool IsImageValid();
            void SetOnClick(std::function<void()> Callback);
            void OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y);
            void OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos);
        protected:
            pu::String img;
            pu::sdl2::Texture ntex;
            s32 x;
            s32 y;
            s32 w;
            s32 h;
            std::function<void()> cb;
            std::chrono::steady_clock::time_point touchtp;
            bool touched;
    };
}