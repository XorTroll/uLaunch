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
        this->icounter = 0;
        this->rawalpha = 255;

        this->bgSuspendedRaw = RawData::New(0, 0, raw, 1280, 720, 4);
        this->bgSuspendedRaw->SetVisible(false);
        this->Add(this->bgSuspendedRaw);

        this->debugMenu = SideMenu::New(pu::ui::Color(0, 120, 255, 255), pu::ui::Color());
        for(auto itm: list.root.titles)
        {
            std::string iconpth;
            if((cfg::TitleType)itm.title_type == cfg::TitleType::Installed) iconpth = cfg::GetTitleCacheIconPath(itm.app_id);
            else if((cfg::TitleType)itm.title_type == cfg::TitleType::Homebrew) iconpth = cfg::GetNROCacheIconPath(itm.nro_path);
            this->debugMenu->AddItem(iconpth);
        }
        for(auto folder: list.folders)
        {
            this->debugMenu->AddItem("romfs:/Test.jpg");
        }
        this->debugMenu->SetOnItemSelected(std::bind(&MenuLayout::menu_Click, this, std::placeholders::_1));
        this->Add(this->debugMenu);

        this->suspendedRaw = RawData::New(0, 0, raw, 1280, 720, 4);
        this->Add(this->suspendedRaw);

        this->SetOnInput(std::bind(&MenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void MenuLayout::menu_Click(u32 index)
    {
        if(index < list.root.titles.size())
        {
            auto title = list.root.titles[index];
            qapp->CreateShowDialog("Title", util::FormatApplicationId(title.app_id), {"Ok"}, true);
            if(!qapp->IsTitleSuspended())
            {
                am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchApplication);
                writer.Write<u64>(title.app_id);
                writer.Write<bool>(false);
                writer.FinishWrite();

                am::QMenuCommandResultReader reader;
                qapp->CreateShowDialog("Command", "0x" + util::FormatApplicationId(reader.GetResult()) + "\n0x" + util::FormatApplicationId(reader.GetReadResult()), {"Ok"}, true);
                if(reader && R_SUCCEEDED(reader.GetReadResult()))
                {
                    qapp->CloseWithFadeOut();
                    return;
                }
                // else qapp->CreateShowDialog("Title launch", "An error ocurred attempting to launch the title:\n")
                reader.FinishRead();
            }
        }
        else
        {
            auto folder = list.folders[index - list.root.titles.size()];
            qapp->CreateShowDialog("Folder", folder.name, {"Ok"}, true);
        }
    }

    void MenuLayout::OnInput(u64 up, u64 down, u64 held, pu::ui::Touch pos)
    {
        if(this->susptr != NULL)
        {
            if(this->icounter >= 0)
            {
                this->icounter++;
                if(this->icounter >= 40)
                {
                    this->icounter = -1;
                }
            }
            else
            {
                if(this->bgSuspendedRaw->IsVisible())
                {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    this->rawalpha += 15;
                    if(this->rawalpha > 175) this->rawalpha = 175;
                }
                else
                {
                    if(this->rawalpha == 0)
                    {
                        this->suspendedRaw->SetVisible(false);
                        this->bgSuspendedRaw->SetVisible(true);
                    }
                    else
                    {
                        this->suspendedRaw->SetAlphaFactor(this->rawalpha);
                        this->rawalpha -= 20;
                        if(this->rawalpha < 0) this->rawalpha = 0;
                    }
                }
            }
        }
    }
}