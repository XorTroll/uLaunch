
#pragma once
#include <ul_Include.hpp>
#include <pu/Plutonium>

namespace ui {

    class RawData : public pu::ui::elm::Element {

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
        
        public:
            RawData(s32 x, s32 y, void *raw, s32 w, s32 h, u32 pix_num);
            PU_SMART_CTOR(RawData)
            ~RawData();

            s32 GetX();
            void SetX(s32 x);
            s32 GetY();
            void SetY(s32 y);
            s32 GetWidth();
            void SetWidth(s32 w);
            s32 GetHeight();
            void SetHeight(s32 h);
            void SetAlphaFactor(u8 Factor);
            void OnRender(pu::ui::render::Renderer::Ref &drawer, s32 x, s32 y);
            void OnInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos);

    };

}