#include <ui/ui_SideMenu.hpp>

namespace ui
{
    SideMenu::SideMenu(pu::ui::Color FocusColor, pu::ui::Color SuspendedColor)
    {
        this->selitm = 0;
        this->suspitm = -1;
        this->selclr = FocusColor;
        this->suspclr = SuspendedColor;
        this->baseiconidx = 0;
        this->movalpha = 0;
        this->leftbicon = NULL;
        this->rightbicon = NULL;
        this->onfocus = [&](u32){};
        this->onclick = [&](u32){};
    }

    s32 SideMenu::GetX()
    {
        return 88;
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
            for(u32 i = 0; i < std::min((size_t)4, this->icons.size()); i++)
            {
                auto icon = pu::ui::render::LoadImage(this->icons[i]);
                this->ricons.push_back(icon);
            }
            this->UpdateBorderIcons();
        }

        u32 basex = X;

        for(u32 i = 0; i < this->ricons.size(); i++)
        {
            auto ricon = this->ricons[i];
            Drawer->RenderTexture(ricon, basex, Y, { -1, ItemSize, ItemSize, -1 });
            pu::ui::Color clr = this->selclr;
            clr.A = 175 - movalpha;
            pu::ui::Color preclr = this->selclr;
            preclr.A = movalpha;
            if((this->baseiconidx + i) == selitm)
            {
                Drawer->RenderRectangleFill(clr, basex, Y, ItemSize, ItemSize);
            }
            else if((this->baseiconidx + i) == preselitm)
            {
                Drawer->RenderRectangleFill(preclr, basex, Y, ItemSize, ItemSize);
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
        else if(Down & KEY_A) (this->onclick)(selitm);
    }

    void SideMenu::SetOnItemSelected(std::function<void(u32)> Fn)
    {
        this->onclick = Fn;
    }

    void SideMenu::SetOnItemFocus(std::function<void(u32)> Fn)
    {
        this->onfocus = Fn;
    }

    void SideMenu::ClearItems()
    {
        this->icons.clear();
        for(auto &ricon: this->ricons) pu::ui::render::DeleteTexture(ricon);
        this->ricons.clear();
        this->selitm = 0;
        this->suspitm = -1;
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
        this->selitm = Index;
    }

    void SideMenu::HandleMoveLeft()
    {
        if(selitm > 0)
        {
            bool ilf = IsLeftFirst();
            preselitm = selitm;
            selitm--;
            if(ilf) ReloadIcons(1);
            else movalpha = 175;
            (this->onfocus)(selitm);
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
            else movalpha = 175;
            (this->onfocus)(selitm);
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
        u32 basex = GetProcessedX();
        for(u32 i = 0; i < this->ricons.size(); i++)
        {
            if((basex == 88) && (this->selitm == (this->baseiconidx + i))) return true;
            basex += ItemSize + Margin;
        }
        return false;
    }
    
    bool SideMenu::IsRightLast()
    {
        if(this->selitm == (this->icons.size() - 1)) return true;
        u32 basex = GetProcessedX();
        for(u32 i = 0; i < this->ricons.size(); i++)
        {
            if((basex == 916) && (this->selitm == (this->baseiconidx + i))) return true;
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

    void SideMenu::UpdateBorderIcons()
    {
        if(leftbicon != NULL) pu::ui::render::DeleteTexture(leftbicon);
        leftbicon = NULL;
        if(baseiconidx > 0)
        {
            leftbicon = pu::ui::render::LoadImage(icons[baseiconidx - 1]);
        }
        if(rightbicon != NULL) pu::ui::render::DeleteTexture(rightbicon);
        rightbicon = NULL;
        if((baseiconidx + 4) < icons.size())
        {
            rightbicon = pu::ui::render::LoadImage(icons[baseiconidx + 4]);
        }
    }
}