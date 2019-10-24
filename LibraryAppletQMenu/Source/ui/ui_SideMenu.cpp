#include <ui/ui_SideMenu.hpp>

namespace ui
{
    SideMenu::SideMenu(pu::ui::Color SuspendedColor, std::string CursorPath)
    {
        this->selitm = 0;
        this->suspitm = -1;
        this->suspclr = SuspendedColor;
        this->baseiconidx = 0;
        this->movalpha = 0;
        this->leftbicon = NULL;
        this->rightbicon = NULL;
        this->cursoricon = pu::ui::render::LoadImage(CursorPath);
        this->onselect = [&](u32,u64){};
        this->onselch = [&](u32){};
    }

    s32 SideMenu::GetX()
    {
        return 98;
    }
    s32 SideMenu::GetY()
    {
        return 294;
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
                this->ricons.push_back(icon);
            }
            this->UpdateBorderIcons();
            (this->onselch)(this->selitm);
        }

        u32 basex = X;

        for(u32 i = 0; i < this->ricons.size(); i++)
        {
            auto ricon = this->ricons[i];
            Drawer->RenderTexture(ricon, basex, Y, { -1, ItemSize, ItemSize, -1 });
            if(this->suspitm >= 0)
            {
                if((this->baseiconidx + i) == (u32)suspitm)
                {
                    Drawer->RenderRectangleFill(this->suspclr, basex, Y + ItemSize + FocusMargin, ItemSize, FocusSize);
                }
            }
            if(this->cursoricon != NULL)
            {
                if((this->baseiconidx + i) == selitm)
                {
                    Drawer->RenderTexture(this->cursoricon, basex - Margin, Y - Margin, { 255 - movalpha, CursorSize, CursorSize, -1 });
                }
                else if((this->baseiconidx + i) == preselitm)
                {
                    Drawer->RenderTexture(this->cursoricon, basex - Margin, Y - Margin, { movalpha, CursorSize, CursorSize, -1 });
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
        for(auto ricon: this->ricons)
        {
            if(ricon != NULL) pu::ui::render::DeleteTexture(ricon);
        }
        this->ricons.clear();
        this->selitm = 0;
        this->baseiconidx = 0;
        this->suspitm = -1;
        this->ClearBorderIcons();
    }

    void SideMenu::AddItem(std::string Icon)
    {
        this->icons.push_back(Icon);
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
                auto icon = pu::ui::render::LoadImage(icons[selitm]);
                this->ricons.insert(this->ricons.begin(), icon);
                baseiconidx--;
                if(this->ricons.size() == 5)
                {
                    pu::ui::render::DeleteTexture(this->ricons.back());
                    this->ricons.pop_back();
                }
                break;
            }
            case 2: // Right
            {
                auto icon = pu::ui::render::LoadImage(icons[selitm]);
                this->ricons.push_back(icon);
                if(this->ricons.size() == 5)
                {
                    pu::ui::render::DeleteTexture(this->ricons.front());
                    this->ricons.erase(this->ricons.begin());
                    baseiconidx++;
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
}