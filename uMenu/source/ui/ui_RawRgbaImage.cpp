#include <ui/ui_RawRgbaImage.hpp>

namespace ui {

    RawRgbaImage::RawRgbaImage(const s32 x, const s32 y, const u8 *rgba_data, const s32 w, const s32 h, const u32 pix_num) : x(x), y(y), w(w), h(h), alpha(0xFF) {
        if(rgba_data != nullptr) {
            this->img_tex = SDL_CreateTexture(pu::ui::render::GetMainRenderer(), SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, this->w, this->h);
            if(this->img_tex != nullptr) {
                SDL_UpdateTexture(this->img_tex, nullptr, rgba_data, w * pix_num);
            }
        }
    }

    RawRgbaImage::~RawRgbaImage() {
        pu::ui::render::DeleteTexture(this->img_tex);
    }

    void RawRgbaImage::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        if(this->img_tex != nullptr) {
            drawer->RenderTexture(this->img_tex, x, y, pu::ui::render::TextureRenderOptions::WithCustomAlphaAndDimensions(this->alpha, this->w, this->h));
        }
    }

}