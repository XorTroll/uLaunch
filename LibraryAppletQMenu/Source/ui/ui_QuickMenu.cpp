#include <ui/ui_QuickMenu.hpp>

namespace ui
{
    QuickMenu::QuickMenu(std::string main_icon)
    {
        this->on = false;
        this->lastheld = 0;
        this->nmain = pu::ui::render::LoadImage(main_icon);
        this->bgalpha = 0;
        this->fgalpha = 0;
        this->off_wait = -1;
    }

    QuickMenu::~QuickMenu()
    {

    }

    void QuickMenu::SetEntry(QuickMenuDirection direction, std::string icon, std::function<void()> on_selected)
    {
        this->RemoveEntry(direction);

        this->item_map[direction] = { on_selected, pu::ui::render::LoadImage(icon) };
    }

    void QuickMenu::RemoveEntry(QuickMenuDirection direction)
    {
        if(this->item_map.count(direction)) this->item_map.erase(direction);
    }

    s32 QuickMenu::GetX()
    {
        return 0;
    }

    s32 QuickMenu::GetY()
    {
        return 0;
    }

    s32 QuickMenu::GetWidth()
    {
        return 1280;
    }

    s32 QuickMenu::GetHeight()
    {
        return 720;
    }

    void QuickMenu::Toggle()
    {
        this->on = !this->on;
    }
    
    bool QuickMenu::IsOn()
    {
        return this->on;
    }

    void QuickMenu::OnRender(pu::ui::render::Renderer::Ref &Drawer, s32 X, s32 Y)
    {
        if(!this->on)
        {
            if(this->off_wait >= 0)
            {
                if(bgalpha > 0)
                {
                    bgalpha -= 20;
                    if(bgalpha < 0) bgalpha = 0;
                }
                if(fgalpha > 0)
                {
                    fgalpha -= 20;
                    if(fgalpha < 0) fgalpha = 0;
                }
            }
            else return;
        }
        else
        {
            if(bgalpha < 220)
            {
                bgalpha += 20;
                if(bgalpha > 220) bgalpha = 220;
            }
            if(fgalpha < 255)
            {
                fgalpha += 20;
                if(fgalpha > 255) fgalpha = 255;
            }
        }

        Drawer->RenderRectangleFill({ 50, 50, 50, (u8)bgalpha }, 0, 0, 1280, 720);

        auto dir = this->GetCurrentDirection();
        Drawer->RenderTexture(this->nmain, MainItemX, MainItemY, { fgalpha, MainItemSize, MainItemSize, -1 });

        for(auto &[direction, subitem]: this->item_map)
        {
            auto [x, y] = this->ComputePositionForDirection(direction);
            auto tex = subitem.nicon;
            auto texw = pu::ui::render::GetTextureWidth(tex);
            auto texh = pu::ui::render::GetTextureHeight(tex);
            x += (SubItemsSize - texw) / 2;
            y += (SubItemsSize - texh) / 2;

            if(direction == dir) SDL_SetTextureColorMod(tex, 150, 150, 200);
            else SDL_SetTextureColorMod(tex, 255, 255, 255);

            Drawer->RenderTexture(tex, x, y, { fgalpha, texw, texh, -1 });
        }
    }

    void QuickMenu::OnInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos)
    {
        auto prevheld = this->lastheld;
        this->lastheld = Held;
        if(this->off_wait >= 0)
        {
            if((this->fgalpha == 0) && (this->bgalpha == 0))
            {
                this->lastheld = (u64)this->off_wait;
                auto dir = this->GetCurrentDirection();
                this->lastheld = Held;
                this->off_wait = -1;

                if(this->item_map.count(dir))
                {
                    auto itm = this->item_map[dir];
                    (itm.on_select)();
                }
            }
        }
        else
        {
            if(Down & KEY_A) this->Toggle();
            else if(Down & KEY_B)
            {
                prevheld = 0;
                this->Toggle();
            }
            
            if(!this->on && (this->bgalpha > 0) && (this->fgalpha > 0)) this->off_wait = prevheld;
        }
    }

    QuickMenuDirection QuickMenu::GetCurrentDirection()
    {
        QuickMenuDirection dir = QuickMenuDirection::None;

        if(this->lastheld & KEY_UP)
        {
            dir = QuickMenuDirection::Up;
            if(this->lastheld & KEY_LEFT) dir = QuickMenuDirection::UpLeft;
            else if(this->lastheld & KEY_RIGHT) dir = QuickMenuDirection::UpRight;
        }
        else if(this->lastheld & KEY_DOWN)
        {
            dir = QuickMenuDirection::Down;
            if(this->lastheld & KEY_LEFT) dir = QuickMenuDirection::DownLeft;
            else if(this->lastheld & KEY_RIGHT) dir = QuickMenuDirection::DownRight;
        }
        else if(this->lastheld & KEY_LEFT) dir = QuickMenuDirection::Left;
        else if(this->lastheld & KEY_RIGHT) dir = QuickMenuDirection::Right;

        return dir;
    }

    std::tuple<s32, s32> QuickMenu::ComputePositionForDirection(QuickMenuDirection direction)
    {
        s32 x = MainItemX;
        s32 y = MainItemY;
        switch(direction)
        {
            case QuickMenuDirection::Up:
                x += ((MainItemSize - SubItemsSize) / 2);
                y -= SubItemsSize;
                break;
            case QuickMenuDirection::Down:
                x += ((MainItemSize - SubItemsSize) / 2);
                y += MainItemSize;
                break;
            case QuickMenuDirection::Left:
                x -= SubItemsSize;
                y += ((MainItemSize - SubItemsSize) / 2);
                break;
            case QuickMenuDirection::Right:
                x += MainItemSize;
                y += ((MainItemSize - SubItemsSize) / 2);
                break;
            case QuickMenuDirection::UpLeft:
                x -= (SubItemsSize - CommonAreaSize);
                y -= (SubItemsSize - CommonAreaSize);
                break;
            case QuickMenuDirection::UpRight:
                x += (MainItemSize - CommonAreaSize);
                y -= (SubItemsSize - CommonAreaSize);
                break;
            case QuickMenuDirection::DownLeft:
                x -= (SubItemsSize - CommonAreaSize);
                y += (MainItemSize - CommonAreaSize);
                break;
            case QuickMenuDirection::DownRight:
                x += (MainItemSize - CommonAreaSize);
                y += (MainItemSize - CommonAreaSize);
                break;
            default:
                break;
        }
        return std::make_tuple(x, y);
    }
}