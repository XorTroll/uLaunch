#include <ui/ui_MenuLayout.hpp>
#include <os/os_Titles.hpp>
#include <util/util_Convert.hpp>
#include <util/util_JSON.hpp>
#include <ui/ui_QMenuApplication.hpp>
#include <os/os_HomeMenu.hpp>
#include <fs/fs_Stdio.hpp>

extern ui::QMenuApplication::Ref qapp;
extern cfg::TitleList list;
extern std::vector<cfg::TitleRecord> homebrew;
extern cfg::MenuConfig config;
extern cfg::ProcessedTheme theme;

namespace ui
{
    MenuLayout::MenuLayout(void *raw, u8 min_alpha, bool hb_mode)
    {
        this->susptr = raw;
        this->mode = 0;
        this->rawalpha = 255;
        this->root_idx = 0;
        this->root_baseidx = 0;
        this->warnshown = false;
        this->minalpha = min_alpha;
        this->homebrew_mode = hb_mode;

        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));

        this->bgSuspendedRaw = RawData::New(0, 0, raw, 1280, 720, 4);
        this->Add(this->bgSuspendedRaw);

        this->menuToggleClickable = ClickableImage::New(0, 200, cfg::ProcessedThemeResource(theme, "ui/ToggleClick.png"));
        this->menuToggleClickable->SetOnClick(std::bind(&MenuLayout::toggle_Click, this));
        this->menuToggleClickable->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->Add(this->menuToggleClickable);

        this->bannerImage = pu::ui::elm::Image::New(0, 585, cfg::ProcessedThemeResource(theme, "ui/BannerInstalled.png"));
        this->Add(this->bannerImage);

        this->itemName = pu::ui::elm::TextBlock::New(40, 610, "", 30);
        this->itemAuthor = pu::ui::elm::TextBlock::New(45, 650, "", 20);
        this->itemVersion = pu::ui::elm::TextBlock::New(45, 675, "", 20);
        this->itemName->SetColor(textclr);
        this->itemAuthor->SetColor(textclr);
        this->itemVersion->SetColor(textclr);
        this->Add(this->itemName);
        this->Add(this->itemAuthor);
        this->Add(this->itemVersion);

        this->itemsMenu = SideMenu::New(pu::ui::Color(0, 255, 120, 255), cfg::ProcessedThemeResource(theme, "ui/Cursor.png"));
        this->MoveFolder("", false);
        
        this->itemsMenu->SetOnItemSelected(std::bind(&MenuLayout::menu_Click, this, std::placeholders::_1, std::placeholders::_2));
        this->itemsMenu->SetOnSelectionChanged(std::bind(&MenuLayout::menu_OnSelected, this, std::placeholders::_1));
        this->Add(this->itemsMenu);
        this->tp = std::chrono::steady_clock::now();

        this->SetBackgroundImage(cfg::ProcessedThemeResource(theme, "ui/Background.png"));
        this->SetOnInput(std::bind(&MenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    void MenuLayout::menu_Click(u64 down, u32 index)
    {
        if(index == 0)
        {
            if(down & KEY_A)
            {
                if(this->homebrew_mode)
                {
                    am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchHomebrewLibApplet);
                    hb::TargetInput ipt = {};
                    strcpy(ipt.nro_path, "sdmc:/hbmenu.nro"); // Launch normal hbmenu
                    strcpy(ipt.argv, "sdmc:/hbmenu.nro");
                    writer.Write<hb::TargetInput>(ipt);
                    writer.FinishWrite();

                    qapp->StopPlayBGM();
                    qapp->CloseWithFadeOut();
                    return;
                }
                else
                {
                    qapp->CreateShowDialog("All", "All titles...", {"Ok"}, true);
                }
            }
        }
        else
        {
            u32 realidx = index - 1;
            if(this->homebrew_mode)
            {
                auto hb = homebrew[realidx];
                if(down & KEY_A)
                {
                    am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchHomebrewLibApplet);
                    writer.Write<hb::TargetInput>(hb.nro_target);
                    writer.FinishWrite();

                    qapp->StopPlayBGM();
                    qapp->CloseWithFadeOut();
                    return;
                }
                else if(down & KEY_X)
                {
                    auto sopt = qapp->CreateShowDialog("Add to menu", "Would you like to add this homebrew to the main menu?", { "Yes", "Cancel" }, true);
                    if(sopt == 0)
                    {
                        if(cfg::ExistsRecord(list, hb)) qapp->CreateShowDialog("Add to menu", "The homebrew is alredy in the main menu.\nNothing was added nor removed.", { "Ok" }, true);
                        else
                        {
                            cfg::SaveRecord(hb);
                            list.root.titles.push_back(hb);
                            qapp->CreateShowDialog("Add to menu", "The homebrew was successfully added to the main menu.", { "Ok" }, true);
                        }
                    }
                }
            }
            else
            {
                auto &folder = cfg::FindFolderByName(list, this->curfolder);
                if(realidx < folder.titles.size())
                {
                    auto title = folder.titles[realidx];
                    if(down & KEY_A)
                    {
                        if(!qapp->IsSuspended())
                        {
                            if((cfg::TitleType)title.title_type == cfg::TitleType::Homebrew)
                            {
                                int sopt = qapp->CreateShowDialog("Homebrew launch", "How would you like to launch this homebrew?\n\nNOTE: Launching as application might involve BAN RISK, so use it at your own risk!", { "Applet", "Application", "Cancel" }, true);
                                if(sopt == 0)
                                {
                                    am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchHomebrewLibApplet);
                                    writer.Write<hb::TargetInput>(title.nro_target);
                                    writer.FinishWrite();

                                    qapp->StopPlayBGM();
                                    qapp->CloseWithFadeOut();
                                    return;
                                }
                                else if(sopt == 1)
                                {
                                    am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchHomebrewApplication);
                                    writer.Write<hb::TargetInput>(title.nro_target);
                                    writer.FinishWrite();

                                    am::QMenuCommandResultReader reader;
                                    if(reader && R_SUCCEEDED(reader.GetReadResult()))
                                    {
                                        qapp->StopPlayBGM();
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
                                am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchApplication);
                                writer.Write<u64>(title.app_id);
                                writer.FinishWrite();

                                am::QMenuCommandResultReader reader;
                                if(reader && R_SUCCEEDED(reader.GetReadResult()))
                                {
                                    qapp->StopPlayBGM();
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
                            if((cfg::TitleType)title.title_type == cfg::TitleType::Homebrew)
                            {
                                if(std::string(title.nro_target.nro_path) == qapp->GetSuspendedHomebrewPath())
                                {
                                    if(this->mode == 1) this->mode = 2;
                                }
                            }
                            else
                            {
                                if(title.app_id == qapp->GetSuspendedApplicationId())
                                {
                                    if(this->mode == 1) this->mode = 2;
                                }
                            }
                        }
                    }
                    else if(down & KEY_X)
                    {
                        if(qapp->IsSuspended())
                        {
                            if((cfg::TitleType)title.title_type == cfg::TitleType::Homebrew)
                            {
                                if(std::string(title.nro_target.nro_path) == qapp->GetSuspendedHomebrewPath()) this->HandleCloseSuspended();
                            }
                            else
                            {
                                if(title.app_id == qapp->GetSuspendedApplicationId()) this->HandleCloseSuspended();
                            }
                        }
                    }
                    else if(down & KEY_Y)
                    {
                        if(this->HandleFolderChange(title))
                        {
                            this->MoveFolder(this->curfolder, true);
                        }
                    }
                }
                else
                {
                    auto foldr = list.folders[realidx - folder.titles.size()];
                    if(down & KEY_A)
                    {
                        this->MoveFolder(foldr.name, true);
                    }
                }
            }
        }
    }

    void MenuLayout::menu_OnSelected(u32 index)
    {
        this->itemAuthor->SetVisible(true);
        this->itemVersion->SetVisible(true);
        if(index > 0)
        {
            u32 realidx = index - 1;
            if(this->homebrew_mode)
            {
                auto hb = homebrew[realidx];
                auto info = cfg::GetRecordInformation(hb);
                auto lent = cfg::GetRecordInformationLanguageEntry(info);
                if(lent != NULL)
                {
                    this->itemName->SetText(lent->name);
                    this->itemAuthor->SetText(lent->author);
                }
                else
                {
                    this->itemName->SetText("Unknown");
                    this->itemAuthor->SetText("Unknown");
                }
                if(strlen(info.nacp.version)) this->itemVersion->SetText(info.nacp.version);
                else this->itemVersion->SetText("0");
                this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerHomebrew.png"));
            }
            else
            {
                auto &folder = cfg::FindFolderByName(list, this->curfolder);
                if(realidx < folder.titles.size())
                {
                    auto title = folder.titles[realidx];
                    auto info = cfg::GetRecordInformation(title);
                    auto lent = cfg::GetRecordInformationLanguageEntry(info);
                    if(lent != NULL)
                    {
                        this->itemName->SetText(lent->name);
                        this->itemAuthor->SetText(lent->author);
                    }
                    else
                    {
                        this->itemName->SetText("Unknown");
                        this->itemAuthor->SetText("Unknown");
                    }
                    if(strlen(info.nacp.version)) this->itemVersion->SetText(info.nacp.version);
                    else this->itemVersion->SetText("0");
                    if((cfg::TitleType)title.title_type == cfg::TitleType::Homebrew) this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerHomebrew.png"));
                    else this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerInstalled.png"));
                }
                else
                {
                    auto foldr = list.folders[realidx - folder.titles.size()];
                    this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerFolder.png"));
                    this->itemAuthor->SetVisible(false);
                    this->itemVersion->SetVisible(false);
                    this->itemName->SetText(foldr.name);
                }
            }
        }
        else
        {
            this->itemAuthor->SetVisible(false);
            this->itemVersion->SetVisible(false);
            
            if(this->homebrew_mode)
            {
                // Since it will launch hbmenu...
                this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerHomebrew.png"));
                this->itemName->SetText("Launch hbmenu");
            }
            else
            {
                // Since it will show "all titles"...
                this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerInstalled.png"));
                this->itemName->SetText("Show all titles");
            }
        }
        if(!this->curfolder.empty()) this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerFolder.png")); // This way user always knows he's inside a folder
    }

    void MenuLayout::MoveFolder(std::string name, bool fade)
    {
        if(fade) qapp->FadeOut();

        if(this->homebrew_mode)
        {
            this->itemsMenu->ClearItems();
            this->itemsMenu->AddItem(cfg::ProcessedThemeResource(theme, "ui/Hbmenu.png"));
            for(auto itm: homebrew)
            {
                this->itemsMenu->AddItem(cfg::GetRecordIconPath(itm));
            }
        }
        else
        {
            if(this->curfolder.empty())
            {
                // Moving from root to a folder, let's save the indexes we were on
                this->root_idx = itemsMenu->GetSelectedItem();
                this->root_baseidx = itemsMenu->GetBaseItemIndex();
            }

            auto &folder = cfg::FindFolderByName(list, name);
            this->itemsMenu->ClearItems();

            // Add first item for all titles menu
            this->itemsMenu->AddItem(cfg::ProcessedThemeResource(theme, "ui/AllTitles.png"));
            u32 tmpidx = 0;
            for(auto itm: folder.titles)
            {
                if((cfg::TitleType)itm.title_type == cfg::TitleType::Installed)
                {
                    if(qapp->IsTitleSuspended()) if(qapp->GetSuspendedApplicationId() == itm.app_id) this->itemsMenu->SetSuspendedItem(tmpidx + 1); // 1st item is always "all titles"!
                }
                else
                {
                    if(qapp->IsHomebrewSuspended()) if(qapp->GetSuspendedHomebrewPath() == std::string(itm.nro_target.nro_path)) this->itemsMenu->SetSuspendedItem(tmpidx + 1); // 1st item is always "all titles"!
                }
                this->itemsMenu->AddItem(cfg::GetRecordIconPath(itm));
                tmpidx++;
            }
            if(name.empty())
            {
                std::vector<cfg::TitleFolder> folders;
                for(auto folder: list.folders)
                {
                    if(!folder.titles.empty())
                    {
                        folders.push_back(folder);
                        this->itemsMenu->AddItem(cfg::ProcessedThemeResource(theme, "ui/Folder.png"));
                    }
                }
                list.folders = folders;
                this->itemsMenu->SetBasePositions(this->root_idx, this->root_baseidx);
            }
            this->itemsMenu->UpdateBorderIcons();

            this->curfolder = name;
        }

        if(fade) qapp->FadeIn();
    }

    void MenuLayout::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos)
    {
        auto ctp = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(ctp - this->tp).count() >= 500)
        {
            if(qapp->LaunchFailed() && !this->warnshown)
            {
                qapp->CreateShowDialog("Title launch", "The title failed to start.\nAre you sure it can be launched? (it isn't deleted, gamecard is inserted...)", {"Ok"}, true);
                this->warnshown = true;
            }
        }

        auto [rc, msg] = am::QMenu_GetLatestQMenuMessage();
        switch(msg)
        {
            case am::QMenuMessage::HomeRequest:
            {
                if(qapp->IsSuspended())
                {
                    if(this->mode == 1) this->mode = 2;
                }
                else while(this->itemsMenu->GetSelectedItem() > 0) this->itemsMenu->HandleMoveLeft();
                break;
            }
            default:
                break;
        }

        if(this->susptr != NULL)
        {
            if(this->mode == 0)
            {
                if(this->rawalpha == this->minalpha)
                {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    this->mode = 1;
                }
                else
                {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    this->rawalpha -= 10;
                    if(this->rawalpha < this->minalpha) this->rawalpha = this->minalpha;
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

        if(down & KEY_B)
        {
            if(!this->curfolder.empty()) this->MoveFolder("", true);
        }
        else if(down & KEY_MINUS)
        {
            SwkbdConfig swkbd;
            swkbdCreate(&swkbd, 0);
            swkbdConfigSetHeaderText(&swkbd, "Enter web page URL");
            char url[500] = {0};
            auto rc = swkbdShow(&swkbd, url, 500);
            if(R_SUCCEEDED(rc))
            {
                WebCommonConfig web = {};
                webPageCreate(&web, url);
                webConfigSetWhitelist(&web, ".*");

                am::QMenuCommandWriter writer(am::QDaemonMessage::OpenWebPage);
                writer.Write<WebCommonConfig>(web);
                writer.FinishWrite();

                qapp->StopPlayBGM();
                qapp->CloseWithFadeOut();
                return;
            }
        }
    }

    void MenuLayout::toggle_Click()
    {
        this->homebrew_mode = !this->homebrew_mode;
        this->MoveFolder("", true);
    }

    bool MenuLayout::HandleFolderChange(cfg::TitleRecord &rec)
    {
        bool changedone = false;

        if(this->curfolder.empty())
        {
            SwkbdConfig swkbd;
            swkbdCreate(&swkbd, 0);
            swkbdConfigSetHeaderText(&swkbd, "Enter directory name");
            char dir[500] = {0};
            auto rc = swkbdShow(&swkbd, dir, 500);
            if(R_SUCCEEDED(rc))
            {
                changedone = cfg::MoveRecordTo(list, rec, std::string(dir));
            }
        }
        else
        {
            auto sopt = qapp->CreateShowDialog("Title move", "Would you like to move this entry outside the folder?", { "Yes", "Cancel" }, true);
            if(sopt == 0)
            {
                changedone = cfg::MoveRecordTo(list, rec, "");
            }
        }

        return changedone;
    }

    void MenuLayout::HandleCloseSuspended()
    {
        auto sopt = qapp->CreateShowDialog("Suspended title", "Would you like to close this title?\nAll unsaved data will be lost.", { "Yes", "Cancel" }, true);
        if(sopt == 0)
        {
            am::QMenuCommandWriter writer(am::QDaemonMessage::TerminateApplication);
            writer.FinishWrite();

            this->itemsMenu->UnsetSuspendedItem();
            qapp->NotifyEndSuspended();
            this->bgSuspendedRaw->SetAlphaFactor(0);
        }
    }
}