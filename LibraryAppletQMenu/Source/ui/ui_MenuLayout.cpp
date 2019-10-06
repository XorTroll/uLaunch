#include <ui/ui_MenuLayout.hpp>
#include <os/os_Titles.hpp>
#include <util/util_Convert.hpp>
#include <ui/ui_QMenuApplication.hpp>
#include <os/os_HomeMenu.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::QMenuApplication::Ref qapp;
extern cfg::TitleList list;

namespace ui
{
    MenuLayout::MenuLayout(void *raw)
    {
        this->susptr = raw;
        this->mode = 0;
        this->rawalpha = 255;

        this->bgSuspendedRaw = RawData::New(0, 0, raw, 1280, 720, 4);
        this->Add(this->bgSuspendedRaw);

        this->itemsMenu = SideMenu::New(pu::ui::Color(0, 120, 255, 255), pu::ui::Color());
        for(auto itm: list.root.titles)
        {
            std::string iconpth;
            if((cfg::TitleType)itm.title_type == cfg::TitleType::Installed) iconpth = cfg::GetTitleCacheIconPath(itm.app_id);
            else if((cfg::TitleType)itm.title_type == cfg::TitleType::Homebrew) iconpth = cfg::GetNROCacheIconPath(itm.nro_path);
            this->itemsMenu->AddItem(iconpth);
        }
        for(auto folder: list.folders)
        {
            this->itemsMenu->AddItem("romfs:/Test.jpg");
        }
        this->itemsMenu->SetOnItemSelected(std::bind(&MenuLayout::menu_Click, this, std::placeholders::_1));
        this->Add(this->itemsMenu);

        this->SetOnInput(std::bind(&MenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void MenuLayout::menu_Click(u32 index)
    {
        if(index < list.root.titles.size())
        {
            auto title = list.root.titles[index];
            if(!qapp->IsTitleSuspended())
            {
                am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchApplication);
                writer.Write<u64>(title.app_id);
                writer.Write<bool>(false);
                writer.FinishWrite();

                am::QMenuCommandResultReader reader;
                if(reader && R_SUCCEEDED(reader.GetReadResult()))
                {
                    qapp->CloseWithFadeOut();
                    return;
                }
                else
                {
                    auto rc = reader.GetReadResult();
                    qapp->CreateShowDialog("Title launch", "An error ocurred attempting to launch the title:\n" + util::FormatResultDisplay(rc) + " (" + util::FormatResultHex(rc) + ")", { "Ok" }, true);
                }
                reader.FinishRead();
            }
        }
        else
        {
            auto folder = list.folders[index - list.root.titles.size()];
            qapp->CreateShowDialog("Folder", folder.name, {"Ok"}, true);
        }
    }

    void MenuLayout::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos)
    {
        auto [rc, msg] = am::QMenu_GetLatestQMenuMessage();
        switch(msg)
        {
            case am::QMenuMessage::HomeRequest:
            {
                if(qapp->IsTitleSuspended())
                {
                    if(this->mode == 1) this->mode = 2;
                }
                else while(this->itemsMenu->GetSelectedItem() > 0) this->itemsMenu->HandleMoveLeft();
                break;
            }
            default:
                break;
        }

        /*
        if(down & KEY_X)
        {
            auto [rc, msg] = am::QMenu_GetLatestQMenuMessage();
            qapp->CreateShowDialog("HOME SWEET HOME", "0x" + util::FormatApplicationId((u32)msg), {"Ok"}, true);
            if(qapp->IsTitleSuspended())
            {
                if(this->mode == 1) this->mode = 2;
            }
            else while(this->itemsMenu->GetSelectedItem() > 0) this->itemsMenu->HandleMoveLeft();
        }
        */

        if(this->susptr != NULL)
        {

            if(this->mode == 0)
            {
                if(this->rawalpha == 80) this->mode = 1;
                else
                {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    this->rawalpha -= 10;
                    if(this->rawalpha < 80) this->rawalpha = 80;
                    else
                    {
                        s32 cw = this->bgSuspendedRaw->GetWidth();
                        s32 ch = this->bgSuspendedRaw->GetHeight();
                        // 16:9 ratio
                        cw -= 16;
                        ch -= 9;
                        s32 x = (1280 - cw) / 2;
                        s32 y = (720 - ch) / 2;
                        this->bgSuspendedRaw->SetX(x);
                        this->bgSuspendedRaw->SetY(y);
                        this->bgSuspendedRaw->SetWidth(cw);
                        this->bgSuspendedRaw->SetHeight(ch);
                    }
                }
            }
            else if(this->mode == 2)
            {
                if(this->rawalpha == 255)
                {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    am::QMenuCommandWriter writer(am::QDaemonMessage::ResumeApplication);
                    writer.FinishWrite();

                    am::QMenuCommandResultReader reader;
                    reader.FinishRead();
                }
                else
                {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    this->rawalpha += 10;
                    if(this->rawalpha > 255) this->rawalpha = 255;
                    else
                    {
                        s32 cw = this->bgSuspendedRaw->GetWidth();
                        s32 ch = this->bgSuspendedRaw->GetHeight();
                        // 16:9 ratio
                        cw += 16;
                        ch += 9;
                        s32 x = (1280 - cw) / 2;
                        s32 y = (720 - ch) / 2;
                        this->bgSuspendedRaw->SetX(x);
                        this->bgSuspendedRaw->SetY(y);
                        this->bgSuspendedRaw->SetWidth(cw);
                        this->bgSuspendedRaw->SetHeight(ch);
                    }
                }
            }
        }
    }
}