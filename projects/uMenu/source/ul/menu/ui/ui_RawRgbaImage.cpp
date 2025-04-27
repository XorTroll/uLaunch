#include <ul/menu/ui/ui_RawRgbaImage.hpp>

namespace ul::menu::ui {

    void RawRgbaImage::ClearImage() {
        pu::ui::render::DeleteTexture(this->img_tex);
    }

    void RawRgbaImage::LoadImage(const u8 *rgba_data, const s32 data_w, const s32 data_h, const u32 pix_num) {
        this->ClearImage();

        this->img_tex = SDL_CreateTexture(pu::ui::render::GetMainRenderer(), SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, data_w, data_h);
        if(this->img_tex != nullptr) {
            this->w = data_w;
            this->h = data_h;
            SDL_UpdateTexture(this->img_tex, nullptr, rgba_data, w * pix_num);
        }

        delete[] rgba_data;
    }

    void RawRgbaImage::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        if(this->img_tex != nullptr) {
            drawer->RenderTexture(this->img_tex, x, y, pu::ui::render::TextureRenderOptions(this->alpha, this->w, this->h, {}, {}, {}));
        }
    }

}
