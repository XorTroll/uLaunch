#include <ul/menu/ui/ui_InputBar.hpp>

namespace ul::menu::ui {

    InputBar::InputBar(const s32 y) : y(y) {
        this->bg_img = pu::ui::render::LoadImage("sdmc:/umad/uitest/input_bar_bg.png");
    }

    void InputBar::ClearInputs() {
        for(auto &[_, text_tex] : this->inputs) {
            pu::ui::render::DeleteTexture(text_tex);
        }
        this->inputs.clear();
    }

    void InputBar::AddSetInput(const u64 key, const std::string &text) {
        if(this->inputs.count(key) > 0) {
            pu::ui::render::DeleteTexture(this->inputs.at(key));
        }

        if(!text.empty()) {
            this->inputs[key] = pu::ui::render::RenderText(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Small), GetKeyString(key) + ") " + text, { 0xFF, 0xFF, 0xFF, 0xFF });
        }
        else {
            this->inputs.erase(key);
        }
    }

    void InputBar::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {
        drawer->RenderTexture(this->bg_img, x, y, pu::ui::render::TextureRenderOptions::WithCustomAlpha(Alpha));

        auto cur_x = x + SideMargin;
        for(const auto &[_, text] : this->inputs) {
            const auto text_y = y + (this->GetHeight() - pu::ui::render::GetTextureHeight(text)) / 2;
            drawer->RenderTexture(text, cur_x, text_y, pu::ui::render::TextureRenderOptions::WithCustomAlpha(Alpha));
            cur_x += pu::ui::render::GetTextureWidth(text) + InputMargin;
        }
    }

}