#include <ui/ui_RawData.hpp>

namespace ui {

    RawData::RawData(s32 x, s32 y, void *raw, s32 w, s32 h, u32 pix_num) : x(x), y(y), w(w), h(h), falpha(0xFF), ptr(raw), pitch(w * pix_num) {
        if(this->ptr != nullptr) {
            this->ntex = SDL_CreateTexture(pu::ui::render::GetMainRenderer(), SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, this->w, this->h);
            if(this->ntex != nullptr) {
                SDL_UpdateTexture(this->ntex, nullptr, this->ptr, this->pitch);
            }
        }
    }

    RawData::~RawData() {
        if(this->ntex != nullptr) {
            pu::ui::render::DeleteTexture(this->ntex);
            this->ntex = nullptr;
        }
    }

    s32 RawData::GetX() {
        return this->x;
    }

    void RawData::SetX(s32 x) {
        this->x = x;
    }

    s32 RawData::GetY() {
        return this->y;
    }

    void RawData::SetY(s32 y) {
        this->y = y;
    }

    s32 RawData::GetWidth() {
        return this->w;
    }

    void RawData::SetWidth(s32 w) {
        this->w = w;
    }

    s32 RawData::GetHeight() {
        return this->h;
    }

    void RawData::SetHeight(s32 h) {
        this->h = h;
    }

    void RawData::SetAlphaFactor(u8 factor) {
        this->falpha = factor;
    }

    void RawData::OnRender(pu::ui::render::Renderer::Ref &drawer, s32 x, s32 y) {
        if(this->ntex != nullptr) {
            drawer->SetBaseRenderAlpha(this->falpha);
            drawer->RenderTexture(this->ntex, x, y, { -1, this->w, this->h, -1 });
            drawer->UnsetBaseRenderAlpha();
        }
    }

    void RawData::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) {}

}