#include <ul/menu/ui/ui_MultiTextBlock.hpp>

namespace ul::menu::ui {

    void MultiTextBlock::UpdatePositionsSizes() {
        s32 w = 0;
        s32 max_h = 0;
        for(auto &tb: this->text_blocks) {
            tb->SetX(this->GetProcessedX() + w);
            tb->SetY(this->GetProcessedY());
            w += tb->GetWidth();
            const auto tb_h = tb->GetHeight();
            if(tb_h > max_h) {
                max_h = tb_h;
            }
        }
        this->w = w;
        this->h = max_h;
    }

    void MultiTextBlock::Add(pu::ui::elm::TextBlock::Ref text) {
        this->text_blocks.push_back(text);
        this->UpdatePositionsSizes();    
    }

    void MultiTextBlock::Clear() {
        this->text_blocks.clear();
        this->w = 0;
        this->h = 0;
    }

    void MultiTextBlock::OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) {}

    void MultiTextBlock::OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {}

}
