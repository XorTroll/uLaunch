#include <ui/ui_SideMenu.hpp>

namespace ui
{
    SideMenu::SideMenu(pu::ui::Color SuspendedColor, std::string CursorPath, std::string SuspendedImagePath, std::string MultiselectImagePath, u32 TextX, u32 TextY, u32 TextSize, pu::ui::Color TextColor, s32 y)
    {
        this->selitm = 0;
        this->suspitm = -1;
        this->suspclr = SuspendedColor;
        this->baseiconidx = 0;
        this->scrollflag = 0;
        this->movalpha = 0;
        this->leftbicon = NULL;
        this->rightbicon = NULL;
        this->cursoricon = pu::ui::render::LoadImage(CursorPath);
        this->suspicon = pu::ui::render::LoadImage(SuspendedImagePath);
        this->mselicon = pu::ui::render::LoadImage(MultiselectImagePath);
        this->textfont = pu::ui::render::LoadDefaultFont(TextSize);
        this->textx = TextX;
        this->texty = TextY;
        this->textclr = TextColor;
        this->onselect = [&](u32,u64){};
        this->onselch = [&](u32){};
        this->scrolltpvalue = 50;
        this->scrollcount = 0;
        this->enabled = true;
        this->SetY(y);
    }

    SideMenu::~SideMenu()
    {
        if(this->cursoricon != NULL)
        {
            pu::ui::render::DeleteTexture(this->cursoricon);
            this->cursoricon = NULL;
        }
        if(this->suspicon != NULL)
        {
            pu::ui::render::DeleteTexture(this->suspicon);
            this->suspicon = NULL;
        }
        pu::ui::render::DeleteFont(this->textfont);
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

    void SideMenu::OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y)
    {
        if(this->icons.empty()) return;
        if(this->ricons.empty())
        {
            for(u32 i = 0; i < std::min((size_t)4, (this->icons.size() - this->baseiconidx)); i++)
            {
                auto icon = pu::ui::render::LoadImage(this->icons[this->baseiconidx + i]);
                auto text = this->icons_texts[this->baseiconidx + i];
                this->ricons.push_back(icon);
                pu::ui::render::NativeTexture ntext = NULL;
                if(!text.empty()) ntext = pu::ui::render::RenderText(this->textfont, text, this->textclr);
                this->ricons_texts.push_back(ntext);
            }
            this->UpdateBorderIcons();
            (this->onselch)(this->selitm);
        }

        u32 basex = X;

        for(u32 i = 0; i < this->ricons.size(); i++)
        {
            auto ricon = this->ricons[i];
            Drawer->RenderTexture(ricon, basex, Y, { -1, ItemSize, ItemSize, -1 });
            auto ntext = this->ricons_texts[i];
            if(ntext != NULL) Drawer->RenderTexture(ntext, basex + this->textx, Y + this->texty);
            if(this->IsItemMultiselected(this->baseiconidx + i)) Drawer->RenderTexture(this->mselicon, basex - Margin, Y - Margin, { -1, ExtraIconSize, ExtraIconSize, -1 });
            if(this->suspitm >= 0)
            {
                if((this->baseiconidx + i) == (u32)suspitm)
                {
                    if(this->suspicon != NULL) Drawer->RenderTexture(this->suspicon, basex - Margin, Y - Margin, { -1, ExtraIconSize, ExtraIconSize, -1 });
                }
            }
            if(this->cursoricon != NULL)
            {
                if((this->baseiconidx + i) == selitm)
                {
                    Drawer->RenderTexture(this->cursoricon, basex - Margin, Y - Margin, { 255 - movalpha, ExtraIconSize, ExtraIconSize, -1 });
                }
                else if((this->baseiconidx + i) == preselitm)
                {
                    Drawer->RenderTexture(this->cursoricon, basex - Margin, Y - Margin, { movalpha, ExtraIconSize, ExtraIconSize, -1 });
                }
            }
            basex += ItemSize + Margin;
        }

        if(leftbicon != NULL)
        {
            Drawer->RenderTexture(leftbicon, X - ItemSize - Margin, Y, { -1, ItemSize, ItemSize, -1 });
        }

        if(rightbicon != NULL)
        {
            Drawer->RenderTexture(rightbicon, X + ((ItemSize + Margin) * 4), Y, { -1, ItemSize, ItemSize, -1 });
        }

        if(movalpha > 0)
        {
            s32 tmpalpha = movalpha - 30;
            if(tmpalpha < 0) tmpalpha = 0;
            movalpha = (u8)tmpalpha;
        }
    }

    void SideMenu::OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Touch)
    {
        if(this->ricons.empty()) return;
        if(!this->enabled) return;

        if(Down & KEY_LEFT) HandleMoveLeft();
        else if(Down & KEY_RIGHT) HandleMoveRight();
        else if(!Touch.IsEmpty())
        {
            s32 basex = this->GetProcessedX();
            s32 basey = this->GetProcessedY();

            if(this->cursoricon != NULL)
            {
                for(u32 i = 0; i < this->ricons.size(); i++)
                {
                    if((Touch.X >= basex) && (Touch.X < (basex + (s32)ItemSize)) && (Touch.Y >= basey) && (Touch.Y < (basey + (s32)ItemSize)))
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
        else (this->onselect)(Down, this->selitm);

        if(Held & KEY_LEFT)
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
        else if(Held & KEY_RIGHT)
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

    void SideMenu::SetOnItemSelected(std::function<void(u64, u32)> Fn)
    {
        this->onselect = Fn;
    }

    void SideMenu::SetOnSelectionChanged(std::function<void(u32)> Fn)
    {
        this->onselch = Fn;
    }

    void SideMenu::ClearItems()
    {
        this->icons.clear();
        this->icons_texts.clear();
        this->icons_mselected.clear();
        for(auto ricon: this->ricons)
        {
            if(ricon != NULL) pu::ui::render::DeleteTexture(ricon);
        }
        this->ricons.clear();
        for(auto rtext: this->ricons_texts)
        {
            if(rtext != NULL) pu::ui::render::DeleteTexture(rtext);
        }
        this->ricons_texts.clear();
        this->selitm = 0;
        this->baseiconidx = 0;
        this->suspitm = -1;
        this->ClearBorderIcons();
    }

    void SideMenu::AddItem(std::string Icon, std::string Text)
    {
        this->icons.push_back(Icon);
        this->icons_texts.push_back(Text);
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

    void SideMenu::SetSelectedItem(u32 Index)
    {
        if(Index < this->icons.size()) this->selitm = Index;
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

    void SideMenu::ReloadIcons(u32 Direction)
    {
        switch(Direction)
        {
            case 1: // Left
            {
                auto icon = pu::ui::render::LoadImage(this->icons[this->selitm]);
                this->ricons.insert(this->ricons.begin(), icon);
                auto text = this->icons_texts[this->selitm];
                pu::ui::render::NativeTexture ntext = NULL;
                if(!text.empty()) ntext = pu::ui::render::RenderText(this->textfont, text, this->textclr);
                this->ricons_texts.insert(this->ricons_texts.begin(), ntext);
                this->baseiconidx--;
                if(this->ricons.size() == 5)
                {
                    pu::ui::render::DeleteTexture(this->ricons.back());
                    this->ricons.pop_back();
                    auto ntext = this->ricons_texts.back();
                    if(ntext != NULL) pu::ui::render::DeleteTexture(ntext);
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
                    if(ntext != NULL) pu::ui::render::DeleteTexture(ntext);
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

    void SideMenu::SetBasePositions(u32 SelectedIdx, u32 BaseIdx)
    {
        if(SelectedIdx < this->icons.size())
        {
            this->SetSelectedItem(SelectedIdx);
            this->SetBaseItemIndex(BaseIdx);
        }
    }

    void SideMenu::ClearBorderIcons()
    {
        if(leftbicon != NULL) pu::ui::render::DeleteTexture(leftbicon);
        leftbicon = NULL;
        if(rightbicon != NULL) pu::ui::render::DeleteTexture(rightbicon);
        rightbicon = NULL;
    }

    void SideMenu::UpdateBorderIcons()
    {
        this->ClearBorderIcons();
        if(baseiconidx > 0)
        {
            leftbicon = pu::ui::render::LoadImage(icons[baseiconidx - 1]);
        }
        if((baseiconidx + 4) < icons.size())
        {
            rightbicon = pu::ui::render::LoadImage(icons[baseiconidx + 4]);
        }
    }

    void SideMenu::ResetMultiselections()
    {
        this->icons_mselected.clear();
        for(u32 i = 0; i < this->icons.size(); i++) this->icons_mselected.push_back(false);
    }

    void SideMenu::SetItemMultiselected(u32 index, bool selected)
    {
        if(index < this->icons_mselected.size())
        {
            this->icons_mselected[index] = selected;
        }
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