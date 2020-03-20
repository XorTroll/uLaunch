#include <ui/ui_SideMenu.hpp>

namespace ui
{
    SideMenu::SideMenu(pu::ui::Color suspended_clr, std::string cursor_path, std::string suspended_img_path, std::string multiselect_img_path, u32 txt_x, u32 txt_y, pu::String font_name, pu::ui::Color txt_clr, s32 y)
    {
        this->selitm = 0;
        this->suspitm = -1;
        this->suspclr = suspended_clr;
        this->baseiconidx = 0;
        this->scrollflag = 0;
        this->movalpha = 0;
        this->leftbicon = nullptr;
        this->rightbicon = nullptr;
        this->cursoricon = pu::ui::render::LoadImage(cursor_path);
        this->suspicon = pu::ui::render::LoadImage(suspended_img_path);
        this->mselicon = pu::ui::render::LoadImage(multiselect_img_path);
        this->textfont = font_name;
        this->textx = txt_x;
        this->texty = txt_y;
        this->textclr = txt_clr;
        this->onselect = [](u32,u64){};
        this->onselch = [](u32){};
        this->scrolltpvalue = 50;
        this->scrollcount = 0;
        this->enabled = true;
        this->SetY(y);
    }

    SideMenu::~SideMenu()
    {
        if(this->cursoricon != nullptr)
        {
            pu::ui::render::DeleteTexture(this->cursoricon);
            this->cursoricon = nullptr;
        }
        if(this->suspicon != nullptr)
        {
            pu::ui::render::DeleteTexture(this->suspicon);
            this->suspicon = nullptr;
        }
        this->ClearItems();
    }

    s32 SideMenu::GetX()
    {
        return 98;
    }

    s32 SideMenu::GetY()
    {
        return this->y;
    }

    void SideMenu::SetX(s32 x)
    {
    }

    void SideMenu::SetY(s32 y)
    {
        this->y = y;
    }

    s32 SideMenu::GetWidth()
    {
        return 1280;
    }
    s32 SideMenu::GetHeight()
    {
        return ItemSize + FocusSize + FocusMargin;
    }

    void SideMenu::OnRender(pu::ui::render::Renderer::Ref &drawer, s32 x, s32 y)
    {
        if(this->icons.empty()) return;
        if(this->ricons.empty())
        {
            for(u32 i = 0; i < std::min((size_t)4, (this->icons.size() - this->baseiconidx)); i++)
            {
                auto icon = pu::ui::render::LoadImage(this->icons[this->baseiconidx + i]);
                auto text = this->icons_texts[this->baseiconidx + i];
                this->ricons.push_back(icon);
                pu::sdl2::Texture ntext = nullptr;
                if(!text.empty()) ntext = pu::ui::render::RenderText(this->textfont, text, this->textclr);
                this->ricons_texts.push_back(ntext);
            }
            this->UpdateBorderIcons();
            (this->onselch)(this->selitm);
        }

        u32 basex = x;

        for(u32 i = 0; i < this->ricons.size(); i++)
        {
            auto ricon = this->ricons[i];
            drawer->RenderTexture(ricon, basex, y, { -1, ItemSize, ItemSize, -1 });
            auto ntext = this->ricons_texts[i];
            if(ntext != nullptr) drawer->RenderTexture(ntext, basex + this->textx, y + this->texty);
            if(this->IsItemMultiselected(this->baseiconidx + i)) drawer->RenderTexture(this->mselicon, basex - Margin, y - Margin, { -1, ExtraIconSize, ExtraIconSize, -1 });
            if(this->suspitm >= 0)
            {
                if((this->baseiconidx + i) == (u32)suspitm)
                {
                    if(this->suspicon != nullptr) drawer->RenderTexture(this->suspicon, basex - Margin, y - Margin, { -1, ExtraIconSize, ExtraIconSize, -1 });
                }
            }
            if(this->cursoricon != nullptr)
            {
                if((this->baseiconidx + i) == selitm)
                {
                    drawer->RenderTexture(this->cursoricon, basex - Margin, y - Margin, { 255 - movalpha, ExtraIconSize, ExtraIconSize, -1 });
                }
                else if((this->baseiconidx + i) == preselitm)
                {
                    drawer->RenderTexture(this->cursoricon, basex - Margin, y - Margin, { movalpha, ExtraIconSize, ExtraIconSize, -1 });
                }
            }
            basex += ItemSize + Margin;
        }

        if(leftbicon != nullptr)
        {
            drawer->RenderTexture(leftbicon, x - ItemSize - Margin, y, { -1, ItemSize, ItemSize, -1 });
        }

        if(rightbicon != nullptr)
        {
            drawer->RenderTexture(rightbicon, x + ((ItemSize + Margin) * 4), y, { -1, ItemSize, ItemSize, -1 });
        }

        if(movalpha > 0)
        {
            s32 tmpalpha = movalpha - 30;
            if(tmpalpha < 0) tmpalpha = 0;
            movalpha = (u8)tmpalpha;
        }
    }

    void SideMenu::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos)
    {
        if(this->ricons.empty()) return;
        if(!this->enabled) return;

        if(down & KEY_LEFT) HandleMoveLeft();
        else if(down & KEY_RIGHT) HandleMoveRight();
        else if(!pos.IsEmpty())
        {
            s32 basex = this->GetProcessedX();
            s32 basey = this->GetProcessedY();

            if(this->cursoricon != nullptr)
            {
                for(u32 i = 0; i < this->ricons.size(); i++)
                {
                    if((pos.X >= basex) && (pos.X < (basex + (s32)ItemSize)) && (pos.Y >= basey) && (pos.Y < (basey + (s32)ItemSize)))
                    {
                        if((this->baseiconidx + i) == selitm) (this->onselect)(KEY_A, this->selitm);
                        else
                        {
                            preselitm = selitm;
                            selitm = this->baseiconidx + i;
                            movalpha = 255;
                            (this->onselch)(this->selitm);
                        }
                        break;
                    }
                    basex += ItemSize + Margin;
                }
            }
        }
        else (this->onselect)(down, this->selitm);

        if(held & KEY_LEFT)
        {
            if(this->scrollflag == 1)
            {
                auto curtp = std::chrono::steady_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(curtp - this->scrolltp).count();
                if(diff >= 300)
                {
                    if(this->scrollmoveflag)
                    {
                        auto diff2 = std::chrono::duration_cast<std::chrono::milliseconds>(curtp - this->scrollmovetp).count();
                        if(diff2 >= this->scrolltpvalue)
                        {
                            if(this->scrollcount >= 5)
                            {
                                this->scrollcount = 0;
                                this->scrolltpvalue /= 2;
                            }
                            this->scrollmoveflag = false;
                            this->HandleMoveLeft();
                            this->scrollcount++;
                        }
                    }
                    else
                    {
                        this->scrollmovetp = std::chrono::steady_clock::now();
                        this->scrollmoveflag = true;
                    }
                }
            }
            else
            {
                this->scrollflag = 1;
                this->scrolltp = std::chrono::steady_clock::now();
            }
        }
        else if(held & KEY_RIGHT)
        {
            if(this->scrollflag == 2)
            {
                auto curtp = std::chrono::steady_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(curtp - this->scrolltp).count();
                if(diff >= 300)
                {
                    if(this->scrollmoveflag)
                    {
                        auto diff2 = std::chrono::duration_cast<std::chrono::milliseconds>(curtp - this->scrollmovetp).count();
                        if(diff2 >= this->scrolltpvalue)
                        {
                            if(this->scrollcount >= 5)
                            {
                                this->scrollcount = 0;
                                this->scrolltpvalue /= 2;
                            }
                            this->scrollmoveflag = false;
                            this->HandleMoveRight();
                            this->scrollcount++;
                        }
                    }
                    else
                    {
                        this->scrollmovetp = std::chrono::steady_clock::now();
                        this->scrollmoveflag = true;
                    }
                }
            }
            else
            {
                this->scrollflag = 2;
                this->scrolltp = std::chrono::steady_clock::now();
            }
        }
        else
        {
            this->scrollflag = 0;
            this->scrollcount = 0;
            this->scrolltpvalue = 50;
        }
    }

    void SideMenu::SetOnItemSelected(std::function<void(u64, u32)> fn)
    {
        this->onselect = fn;
    }

    void SideMenu::SetOnSelectionChanged(std::function<void(u32)> fn)
    {
        this->onselch = fn;
    }

    void SideMenu::ClearItems()
    {
        this->icons.clear();
        this->icons_texts.clear();
        this->icons_mselected.clear();
        for(auto ricon: this->ricons)
        {
            if(ricon != nullptr) pu::ui::render::DeleteTexture(ricon);
        }
        this->ricons.clear();
        for(auto rtext: this->ricons_texts)
        {
            if(rtext != nullptr) pu::ui::render::DeleteTexture(rtext);
        }
        this->ricons_texts.clear();
        this->selitm = 0;
        this->baseiconidx = 0;
        this->suspitm = -1;
        this->ClearBorderIcons();
    }

    void SideMenu::AddItem(const std::string &icon, const std::string &txt)
    {
        this->icons.push_back(icon);
        this->icons_texts.push_back(txt);
        this->icons_mselected.push_back(false);
    }

    void SideMenu::SetSuspendedItem(u32 Index)
    {
        this->suspitm = Index;
    }

    void SideMenu::UnsetSuspendedItem()
    {
        this->suspitm = -1;
    }

    void SideMenu::SetSelectedItem(u32 idx)
    {
        if(idx < this->icons.size()) this->selitm = idx;
    }

    void SideMenu::HandleMoveLeft()
    {
        if(selitm > 0)
        {
            bool ilf = IsLeftFirst();
            preselitm = selitm;
            selitm--;
            if(ilf) ReloadIcons(1);
            else movalpha = 255;
            (this->onselch)(this->selitm);
        }
    }

    void SideMenu::HandleMoveRight()
    {
        if((selitm + 1) < this->icons.size())
        {
            bool irl = IsRightLast();
            preselitm = selitm;
            selitm++;
            if(irl) ReloadIcons(2);
            else movalpha = 255;
            (this->onselch)(this->selitm);
        }
    }
    
    int SideMenu::GetSuspendedItem()
    {
        return this->suspitm;
    }

    u32 SideMenu::GetSelectedItem()
    {
        return this->selitm;
    }

    bool SideMenu::IsLeftFirst()
    {
        s32 basex = GetProcessedX();
        for(u32 i = 0; i < this->ricons.size(); i++)
        {
            if((basex == this->GetX()) && (this->selitm == (this->baseiconidx + i))) return true;
            basex += ItemSize + Margin;
        }
        return false;
    }
    
    bool SideMenu::IsRightLast()
    {
        if(this->selitm == (this->icons.size() - 1)) return true;
        s32 basex = GetProcessedX();
        for(u32 i = 0; i < this->ricons.size(); i++)
        {
            if((basex == 926) && (this->selitm == (this->baseiconidx + i))) return true;
            basex += ItemSize + Margin;
        }
        return false;
    }

    void SideMenu::ReloadIcons(u32 dir)
    {
        switch(dir)
        {
            case 1: // Left
            {
                auto icon = pu::ui::render::LoadImage(this->icons[this->selitm]);
                this->ricons.insert(this->ricons.begin(), icon);
                auto text = this->icons_texts[this->selitm];
                pu::sdl2::Texture ntext = nullptr;
                if(!text.empty()) ntext = pu::ui::render::RenderText(this->textfont, text, this->textclr);
                this->ricons_texts.insert(this->ricons_texts.begin(), ntext);
                this->baseiconidx--;
                if(this->ricons.size() == 5)
                {
                    pu::ui::render::DeleteTexture(this->ricons.back());
                    this->ricons.pop_back();
                    auto ntext = this->ricons_texts.back();
                    if(ntext != nullptr) pu::ui::render::DeleteTexture(ntext);
                    this->ricons_texts.pop_back();
                }
                break;
            }
            case 2: // Right
            {
                auto icon = pu::ui::render::LoadImage(this->icons[this->selitm]);
                this->ricons.push_back(icon);
                auto ntext = pu::ui::render::RenderText(this->textfont, this->icons_texts[this->selitm], this->textclr);
                this->ricons_texts.push_back(ntext);
                if(this->ricons.size() == 5)
                {
                    pu::ui::render::DeleteTexture(this->ricons.front());
                    this->ricons.erase(this->ricons.begin());
                    auto ntext = this->ricons_texts.front();
                    if(ntext != nullptr) pu::ui::render::DeleteTexture(ntext);
                    this->ricons_texts.erase(this->ricons_texts.begin());
                    this->baseiconidx++;
                }
                break;
            }
        }
        this->UpdateBorderIcons();
    }

    u32 SideMenu::GetBaseItemIndex()
    {
        return this->baseiconidx;
    }

    void SideMenu::SetBaseItemIndex(u32 index)
    {
        this->baseiconidx = index;
    }

    void SideMenu::SetBasePositions(u32 selected_idx, u32 base_idx)
    {
        if(selected_idx < this->icons.size())
        {
            this->SetSelectedItem(selected_idx);
            this->SetBaseItemIndex(base_idx);
        }
    }

    void SideMenu::ClearBorderIcons()
    {
        if(this->leftbicon != nullptr) pu::ui::render::DeleteTexture(this->leftbicon);
        this->leftbicon = nullptr;
        if(this->rightbicon != nullptr) pu::ui::render::DeleteTexture(this->rightbicon);
        this->rightbicon = nullptr;
    }

    void SideMenu::UpdateBorderIcons()
    {
        this->ClearBorderIcons();
        if(this->baseiconidx > 0) this->leftbicon = pu::ui::render::LoadImage(this->icons[this->baseiconidx - 1]);
        if((this->baseiconidx + 4) < this->icons.size()) this->rightbicon = pu::ui::render::LoadImage(this->icons[this->baseiconidx + 4]);
    }

    void SideMenu::ResetMultiselections()
    {
        this->icons_mselected.clear();
        for(u32 i = 0; i < this->icons.size(); i++) this->icons_mselected.push_back(false);
    }

    void SideMenu::SetItemMultiselected(u32 index, bool selected)
    {
        if(index < this->icons_mselected.size()) this->icons_mselected[index] = selected;
    }

    bool SideMenu::IsItemMultiselected(u32 index)
    {
        if(index < this->icons_mselected.size()) return this->icons_mselected[index];
        return false;
    }

    bool SideMenu::IsAnyMultiselected()
    {
        bool any = false;
        for(auto msel: this->icons_mselected)
        {
            if(msel)
            {
                any = true;
                break;
            }
        }
        return any;
    }

    void SideMenu::SetEnabled(bool enabled)
    {
        this->enabled = enabled;
    }
}