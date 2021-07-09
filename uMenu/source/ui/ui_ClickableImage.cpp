
// Grabbed from Goldleaf's source code, slightly edited to work here

#include <ui/ui_ClickableImage.hpp>
#include <fs/fs_Stdio.hpp>

namespace ui {

    s32 ClickableImage::GetX() {
        return this->x;
    }

    void ClickableImage::SetX(s32 x) {
        this->x = x;
    }

    s32 ClickableImage::GetY() {
        return this->y;
    }

    void ClickableImage::SetY(s32 y) {
        this->y = y;
    }

    s32 ClickableImage::GetWidth() {
        return this->w;
    }

    void ClickableImage::SetWidth(s32 w) {
        this->w = w;
    }

    s32 ClickableImage::GetHeight() {
        return this->h;
    }

    void ClickableImage::SetHeight(s32 h) {
        this->h = h;
    }

    std::string ClickableImage::GetImage() {
        return this->img;
    }

    void ClickableImage::SetImage(const std::string &img) {
        this->Dispose();
        if(fs::ExistsFile(img)) {
            this->img = img;
            this->ntex = pu::ui::render::LoadImage(img);
            this->w = pu::ui::render::GetTextureWidth(this->ntex);
            this->h = pu::ui::render::GetTextureHeight(this->ntex);
        }
    }

    bool ClickableImage::IsImageValid() {
        return (ntex != nullptr) && !this->img.empty();
    }

    void ClickableImage::SetOnClick(std::function<void()> cb) {
        this->cb = cb;
    }

    void ClickableImage::OnRender(pu::ui::render::Renderer::Ref &drawer, s32 x, s32 y) {
        drawer->RenderTexture(this->ntex, x, y, { -1, w, h, -1 });
    }

    void ClickableImage::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) {
        if(this->touched) {
            auto tpnow = std::chrono::steady_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(tpnow - this->touchtp).count();
            if(diff >= 200) {
                this->touched = false;
                (this->cb)();
                SDL_SetTextureColorMod(this->ntex, 255, 255, 255);
            }
        }
        else if(!touch_pos.IsEmpty()) {
            if((touch_pos.X >= this->GetProcessedX()) && (touch_pos.X < (this->GetProcessedX() + w)) && (touch_pos.Y >= this->GetProcessedY()) && (touch_pos.Y < (this->GetProcessedY() + h))) {
                this->touchtp = std::chrono::steady_clock::now();
                this->touched = true;
                SDL_SetTextureColorMod(this->ntex, 200, 200, 255);
            }
        }
    }

}