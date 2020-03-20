
#pragma once
#include <ul_Include.hpp>
#include <pu/Plutonium>

namespace ui
{
    class RawData : public pu::ui::elm::Element
    {
        public:
            RawData(s32 X, s32 Y, void *raw, s32 Width, s32 Height, u32 PixNum);
            PU_SMART_CTOR(RawData)
            ~RawData();

            s32 GetX();
            void SetX(s32 X);
            s32 GetY();
            void SetY(s32 Y);
            s32 GetWidth();
            void SetWidth(s32 Width);
            s32 GetHeight();
            void SetHeight(s32 Height);
            void SetAlphaFactor(u8 Factor);
            void OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y);
            void OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos);
        protected:
            s32 x;
            s32 y;
            s32 w;
            s32 h;
            pu::sdl2::Texture ntex;
            u8 falpha;
            void *ptr;
            size_t fullsz;
            size_t pitch;
    };
}