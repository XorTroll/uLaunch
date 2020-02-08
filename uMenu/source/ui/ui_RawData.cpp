#include <ui/ui_RawData.hpp>

namespace ui
{
    RawData::RawData(s32 X, s32 Y, void *raw, s32 Width, s32 Height, u32 PixNum)
    {
        this->x = X;
        this->y = Y;
        this->w = Width;
        this->h = Height;
        this->pitch = Width * PixNum;
        this->ptr = raw;
        this->falpha = 255;
        if(raw != nullptr)
        {
            this->ntex = SDL_CreateTexture(pu::ui::render::GetMainRenderer(), SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, Width, Height);
            if(this->ntex != nullptr)
            {
                SDL_UpdateTexture(this->ntex, nullptr, raw, this->pitch);
            }
        }
    }

    RawData::~RawData()
    {
        if(this->ntex != nullptr)
        {
            pu::ui::render::DeleteTexture(this->ntex);
            this->ntex = nullptr;
        }
    }

    s32 RawData::GetX()
    {
        return this->x;
    }

    void RawData::SetX(s32 X)
    {
        this->x = X;
    }

    s32 RawData::GetY()
    {
        return this->y;
    }

    void RawData::SetY(s32 Y)
    {
        this->y = Y;
    }

    s32 RawData::GetWidth()
    {
        return this->w;
    }

    void RawData::SetWidth(s32 Width)
    {
        this->w = Width;
    }

    s32 RawData::GetHeight()
    {
        return this->h;
    }

    void RawData::SetHeight(s32 Height)
    {
        this->h = Height;
    }

    void RawData::SetAlphaFactor(u8 Factor)
    {
        this->falpha = Factor;
    }

    void RawData::OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y)
    {
        if(this->ntex != nullptr)
        {
            Drawer->SetBaseRenderAlpha(this->falpha);
            Drawer->RenderTexture(this->ntex, X, Y, { -1, this->w, this->h, -1 });
            Drawer->UnsetBaseRenderAlpha();
        }
    }

    void RawData::OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos)
    {
    }
}