#include <ui/ui_MenuLayout.hpp>
#include <os/os_Titles.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <util/util_Misc.hpp>
#include <os/os_Misc.hpp>
#include <am/am_LibraryApplet.hpp>
#include <ui/ui_QMenuApplication.hpp>
#include <os/os_HomeMenu.hpp>
#include <fs/fs_Stdio.hpp>
#include <net/net_Service.hpp>

extern ui::QMenuApplication::Ref qapp;
extern cfg::TitleList list;
extern std::vector<cfg::TitleRecord> homebrew;
extern cfg::Config config;
extern cfg::ProcessedTheme theme;

namespace ui
{
    MenuLayout::MenuLayout(void *raw, u8 min_alpha)
    {
        this->susptr = raw;
        this->mode = 0;
        this->rawalpha = 255;
        this->last_hasconn = false;
        this->last_batterylvl = 0;
        this->last_charge = false;
        this->warnshown = false;
        this->minalpha = min_alpha;
        this->homebrew_mode = false;
        this->select_on = false;

        pu::ui::Color textclr = pu::ui::Color::FromHex(qapp->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));

        this->bgSuspendedRaw = RawData::New(0, 0, raw, 1280, 720, 4);
        this->Add(this->bgSuspendedRaw);

        // Load banners first
        this->topMenuImage = pu::ui::elm::Image::New(40, 35, cfg::ProcessedThemeResource(theme, "ui/TopMenu.png"));
        qapp->ApplyConfigForElement("main_menu", "top_menu_bg", this->topMenuImage);
        this->Add(this->topMenuImage);

        this->bannerImage = pu::ui::elm::Image::New(0, 585, cfg::ProcessedThemeResource(theme, "ui/BannerInstalled.png"));
        qapp->ApplyConfigForElement("main_menu", "banner_image", this->bannerImage);
        this->Add(this->bannerImage);

        // Then load buttons and other UI elements
        this->logo = ClickableImage::New(610, 13 + 35, "romfs:/Logo.png");
        this->logo->SetWidth(60);
        this->logo->SetHeight(60);
        this->logo->SetOnClick(std::bind(&MenuLayout::logo_Click, this));
        qapp->ApplyConfigForElement("main_menu", "logo_icon", this->logo, false); // Sorry theme makers... logo must be visible, but can be moved
        this->Add(this->logo);

        this->connIcon = pu::ui::elm::Image::New(80, 53, cfg::ProcessedThemeResource(theme, "ui/NoConnectionIcon.png"));
        qapp->ApplyConfigForElement("main_menu", "connection_icon", this->connIcon);
        this->Add(this->connIcon);
        this->users = ClickableImage::New(270, 53, ""); // On layout creation, no user is still selected...
        this->users->SetOnClick(std::bind(&MenuLayout::users_Click, this));
        qapp->ApplyConfigForElement("main_menu", "user_icon", this->users);
        this->Add(this->users);
        this->web = ClickableImage::New(340, 53, cfg::ProcessedThemeResource(theme, "ui/WebIcon.png"));
        this->web->SetOnClick(std::bind(&MenuLayout::web_Click, this));
        qapp->ApplyConfigForElement("main_menu", "web_icon", this->web);
        this->Add(this->web);

        auto curtime = util::GetCurrentTime();
        this->timeText = pu::ui::elm::TextBlock::New(515, 68, curtime);
        this->timeText->SetColor(textclr);
        qapp->ApplyConfigForElement("main_menu", "time_text", this->timeText);
        this->Add(this->timeText);
        auto lvl = util::GetBatteryLevel();
        auto lvlstr = std::to_string(lvl) + "%";
        this->batteryText = pu::ui::elm::TextBlock::New(700, 55, lvlstr);
        this->batteryText->SetColor(textclr);
        qapp->ApplyConfigForElement("main_menu", "battery_text", this->batteryText);
        this->Add(this->batteryText);
        this->batteryIcon = pu::ui::elm::Image::New(700, 80, cfg::ProcessedThemeResource(theme, "ui/BatteryNormalIcon.png"));
        qapp->ApplyConfigForElement("main_menu", "battery_icon", this->batteryIcon);
        this->Add(this->batteryIcon);

        this->settings = ClickableImage::New(880, 53, cfg::ProcessedThemeResource(theme, "ui/SettingsIcon.png"));
        this->settings->SetOnClick(std::bind(&MenuLayout::settings_Click, this));
        qapp->ApplyConfigForElement("main_menu", "settings_icon", this->settings);
        this->Add(this->settings);
        this->themes = ClickableImage::New(950, 53, cfg::ProcessedThemeResource(theme, "ui/ThemesIcon.png"));
        this->themes->SetOnClick(std::bind(&MenuLayout::themes_Click, this));
        qapp->ApplyConfigForElement("main_menu", "themes_icon", this->themes);
        this->Add(this->themes);

        this->fwText = pu::ui::elm::TextBlock::New(1140, 68, os::GetFirmwareVersion());
        this->fwText->SetColor(textclr);
        qapp->ApplyConfigForElement("main_menu", "firmware_text", this->fwText);
        this->Add(this->fwText);

        this->menuToggle = ClickableImage::New(520, 200, cfg::ProcessedThemeResource(theme, "ui/ToggleClick.png"));
        this->menuToggle->SetOnClick(std::bind(&MenuLayout::menuToggle_Click, this));
        qapp->ApplyConfigForElement("main_menu", "menu_toggle_button", this->menuToggle);
        this->Add(this->menuToggle);

        this->itemName = pu::ui::elm::TextBlock::New(40, 610, "", 30);
        this->itemName->SetColor(textclr);
        qapp->ApplyConfigForElement("main_menu", "banner_name_text", this->itemName);
        this->Add(this->itemName);
        this->itemAuthor = pu::ui::elm::TextBlock::New(45, 650, "", 20);
        this->itemAuthor->SetColor(textclr);
        qapp->ApplyConfigForElement("main_menu", "banner_author_text", this->itemAuthor);
        this->Add(this->itemAuthor);
        this->itemVersion = pu::ui::elm::TextBlock::New(45, 675, "", 20);
        this->itemVersion->SetColor(textclr);
        qapp->ApplyConfigForElement("main_menu", "banner_version_text", this->itemVersion);
        this->Add(this->itemVersion);

        this->itemsMenu = SideMenu::New(pu::ui::Color(0, 255, 120, 255), cfg::ProcessedThemeResource(theme, "ui/Cursor.png"), cfg::ProcessedThemeResource(theme, "ui/Suspended.png"), cfg::ProcessedThemeResource(theme, "ui/Multiselect.png"), 294);
        this->MoveFolder("", false);
        this->itemsMenu->SetOnItemSelected(std::bind(&MenuLayout::menu_Click, this, std::placeholders::_1, std::placeholders::_2));
        this->itemsMenu->SetOnSelectionChanged(std::bind(&MenuLayout::menu_OnSelected, this, std::placeholders::_1));
        qapp->ApplyConfigForElement("main_menu", "items_menu", this->itemsMenu, false); // Main menu must be visible, and only Y is customizable here
        this->Add(this->itemsMenu);
        this->tp = std::chrono::steady_clock::now();

        this->sfxTitleLaunch = pu::audio::Load(cfg::ProcessedThemeResource(theme, "sound/TitleLaunch.wav"));
        this->sfxMenuToggle = pu::audio::Load(cfg::ProcessedThemeResource(theme, "sound/MenuToggle.wav"));

        this->SetBackgroundImage(cfg::ProcessedThemeResource(theme, "ui/Background.png"));
        this->SetOnInput(std::bind(&MenuLayout::OnInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }

    MenuLayout::~MenuLayout()
    {
        pu::audio::DeleteSfx(this->sfxTitleLaunch);
        pu::audio::DeleteSfx(this->sfxMenuToggle);
    }

    void MenuLayout::menu_Click(u64 down, u32 index)
    {
        if(this->select_on)
        {
            if(down & KEY_B)
            {
                this->select_on = false;
                this->itemsMenu->ResetMultiselections();
            }
            else if(down & KEY_Y)
            {
                bool selectable = false;
                if(this->homebrew_mode) selectable = true;
                else
                {
                    if(this->curfolder.empty())
                    {
                        bool is_dir = (index < list.folders.size());
                        if(!is_dir) selectable = true;
                    }
                    else selectable = true;
                }
                if(selectable)
                {
                    this->itemsMenu->SetItemMultiselected(index, !this->itemsMenu->IsItemMultiselected(index));
                }
            }
            else if(down & KEY_A)
            {
                if(this->homebrew_mode)
                {
                    auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "multiselect"), cfg::GetLanguageString(config.main_lang, config.default_lang, "hb_mode_entries_add"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "no"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
                    if(sopt == 0)
                    {
                        // Get the idx of the last homebrew element.
                        s32 hbidx = 0;
                        for(auto &entry: list.root.titles)
                        {
                            if((cfg::TitleType)entry.title_type == cfg::TitleType::Installed) break;
                            hbidx++;
                        }
                        if(hbidx < 0) hbidx = 0;
                        bool any = false;
                        for(u32 i = 0; i < homebrew.size(); i++)
                        {
                            auto &hb = homebrew[i];
                            u32 idx = i + 1;
                            if(this->itemsMenu->IsItemMultiselected(idx))
                            {
                                if(!cfg::ExistsRecord(list, hb))
                                {
                                    cfg::SaveRecord(hb);
                                    list.root.titles.insert(list.root.titles.begin() + hbidx, hb);
                                    any = true;
                                    hbidx++;
                                }
                            }
                        }
                        if(any) qapp->ShowNotification(cfg::GetLanguageString(config.main_lang, config.default_lang, "hb_mode_entries_added"));
                        this->select_on = false;
                        this->itemsMenu->ResetMultiselections();
                    }
                    else if(sopt == 1)
                    {
                        this->select_on = false;
                        this->itemsMenu->ResetMultiselections();
                    }
                }
                else if(this->curfolder.empty())
                {
                    auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "multiselect"), cfg::GetLanguageString(config.main_lang, config.default_lang, "menu_move_to_folder"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "no"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
                    if(sopt == 0)
                    {
                        SwkbdConfig swkbd;
                        swkbdCreate(&swkbd, 0);
                        swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(config.main_lang, config.default_lang, "swkbd_new_folder_guide").c_str());
                        char dir[500] = {0};
                        auto rc = swkbdShow(&swkbd, dir, 500);
                        swkbdClose(&swkbd);
                        bool chd = false;
                        if(R_SUCCEEDED(rc))
                        {
                            for(u32 i = 0; i < list.root.titles.size(); i++)
                            {
                                if(i >= list.root.titles.size()) break;
                                auto &title = list.root.titles[i];
                                u32 idx = list.folders.size() + i;
                                if(this->itemsMenu->IsItemMultiselected(idx))
                                {
                                    auto nextmsel = this->itemsMenu->IsItemMultiselected(idx + 1);
                                    bool ok = cfg::MoveRecordTo(list, title, dir);
                                    if(ok)
                                    {
                                        if(nextmsel) i--;
                                        chd = true;
                                    }
                                }
                            }
                        }
                        this->select_on = false;
                        this->itemsMenu->ResetMultiselections();
                        if(chd) this->MoveFolder(this->curfolder, true);
                    }
                    else if(sopt == 1)
                    {
                        this->select_on = false;
                        this->itemsMenu->ResetMultiselections();
                    }
                }
                else
                {
                    auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "multiselect"), cfg::GetLanguageString(config.main_lang, config.default_lang, "menu_move_from_folder"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "no"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
                    if(sopt == 0)
                    {
                        u32 rmvd = 0;
                        auto &folder = cfg::FindFolderByName(list, this->curfolder);
                        auto basesz = folder.titles.size();
                        for(u32 i = 0; i < basesz; i++)
                        {
                            auto &title = folder.titles[i - rmvd];
                            if(this->itemsMenu->IsItemMultiselected(i))
                            {
                                if(cfg::MoveRecordTo(list, title, ""))
                                {
                                    rmvd++;
                                }
                            }
                        }
                        this->select_on = false;
                        this->itemsMenu->ResetMultiselections();
                        this->MoveFolder(folder.titles.empty() ? "" : this->curfolder, true);
                    }
                    else if(sopt == 1)
                    {
                        this->select_on = false;
                        this->itemsMenu->ResetMultiselections();
                    }
                }
            }
        }
        else
        {
            if((down & KEY_A) || (down & KEY_X) || (down & KEY_Y))
            {
                if((index == 0) && this->homebrew_mode)
                {
                    if(down & KEY_A)
                    {
                        am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchHomebrewLibApplet);
                        hb::TargetInput ipt = {};
                        strcpy(ipt.nro_path, "sdmc:/hbmenu.nro"); // Launch normal hbmenu
                        strcpy(ipt.argv, "sdmc:/hbmenu.nro");
                        writer.Write<hb::TargetInput>(ipt);
                        writer.FinishWrite();

                        pu::audio::Play(this->sfxTitleLaunch);
                        qapp->StopPlayBGM();
                        qapp->CloseWithFadeOut();
                        return;
                    }
                }
                else
                {
                    u32 realidx = index;
                    if(this->homebrew_mode) realidx--;
                    if(this->homebrew_mode)
                    {
                        auto hb = homebrew[realidx];
                        if(down & KEY_A)
                        {
                            bool hblaunch = true;
                            if(qapp->IsHomebrewSuspended())
                            {
                                if(std::string(hb.nro_target.nro_path) == qapp->GetSuspendedHomebrewPath())
                                {
                                    if(this->mode == 1) this->mode = 2;
                                    hblaunch = false;
                                }
                                else
                                {
                                    hblaunch = false;
                                    this->HandleCloseSuspended();
                                    hblaunch = !qapp->IsHomebrewSuspended();
                                }
                            }
                            if(hblaunch) this->HandleHomebrewLaunch(hb);
                        }
                        else if(down & KEY_X)
                        {
                            if(qapp->IsSuspended())
                            {
                                if(std::string(hb.nro_target.nro_path) == qapp->GetSuspendedHomebrewPath()) this->HandleCloseSuspended();
                            }
                        }
                        else if(down & KEY_Y)
                        {
                            if(!this->select_on) this->select_on = true;
                            this->itemsMenu->SetItemMultiselected(this->itemsMenu->GetSelectedItem(), true);
                        }
                    }
                    else
                    {
                        auto &folder = cfg::FindFolderByName(list, this->curfolder);
                        s32 titleidx = realidx;
                        if(this->curfolder.empty())
                        {
                            if(realidx >= list.folders.size())
                            {
                                titleidx -= list.folders.size();
                            }
                            else
                            {
                                auto foldr = list.folders[realidx];
                                if(down & KEY_A)
                                {
                                    this->MoveFolder(foldr.name, true);
                                }
                                titleidx = -1;
                            }
                        }
                        if(titleidx >= 0)
                        {
                            auto title = folder.titles[titleidx];
                            if(down & KEY_A)
                            {
                                bool titlelaunch = true;

                                if(qapp->IsSuspended())
                                {
                                    if((cfg::TitleType)title.title_type == cfg::TitleType::Homebrew)
                                    {
                                        if(std::string(title.nro_target.nro_path) == qapp->GetSuspendedHomebrewPath())
                                        {
                                            if(this->mode == 1) this->mode = 2;
                                            titlelaunch = false;
                                        }
                                    }
                                    else if((cfg::TitleType)title.title_type == cfg::TitleType::Installed)
                                    {
                                        if(title.app_id == qapp->GetSuspendedApplicationId())
                                        {
                                            if(this->mode == 1) this->mode = 2;
                                            titlelaunch = false;
                                        }
                                    }
                                    if(titlelaunch)
                                    {
                                        titlelaunch = false;
                                        this->HandleCloseSuspended();
                                        titlelaunch = !qapp->IsSuspended();
                                    }
                                }
                                if(titlelaunch)
                                {
                                    if((cfg::TitleType)title.title_type == cfg::TitleType::Homebrew) this->HandleHomebrewLaunch(title);
                                    else
                                    {
                                        am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchApplication);
                                        writer.Write<u64>(title.app_id);
                                        writer.FinishWrite();

                                        am::QMenuCommandResultReader reader;
                                        if(reader && R_SUCCEEDED(reader.GetReadResult()))
                                        {
                                            pu::audio::Play(this->sfxTitleLaunch);
                                            qapp->StopPlayBGM();
                                            qapp->CloseWithFadeOut();
                                            return;
                                        }
                                        else
                                        {
                                            auto rc = reader.GetReadResult();
                                            qapp->ShowNotification(cfg::GetLanguageString(config.main_lang, config.default_lang, "app_launch_error") + ": " + util::FormatResult(rc));
                                        }
                                        reader.FinishRead();
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
                                if((cfg::TitleType)title.title_type == cfg::TitleType::Homebrew)
                                {
                                    auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "entry_options"), cfg::GetLanguageString(config.main_lang, config.default_lang, "entry_action"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "entry_move"), cfg::GetLanguageString(config.main_lang, config.default_lang, "entry_remove"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
                                    if(sopt == 0)
                                    {
                                        if(!this->select_on) this->select_on = true;
                                        this->itemsMenu->SetItemMultiselected(this->itemsMenu->GetSelectedItem(), true);
                                    }
                                    else if(sopt == 1)
                                    {
                                        auto sopt2 = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "entry_remove"), cfg::GetLanguageString(config.main_lang, config.default_lang, "entry_remove_conf"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "no") }, true);
                                        if(sopt2 == 0)
                                        {
                                            cfg::RemoveRecord(title);
                                            folder.titles.erase(folder.titles.begin() + titleidx);
                                            qapp->ShowNotification(cfg::GetLanguageString(config.main_lang, config.default_lang, "entry_remove_ok"));
                                            this->MoveFolder(this->curfolder, true);
                                        }
                                    }
                                }
                                else
                                {
                                    if(!this->select_on) this->select_on = true;
                                    this->itemsMenu->SetItemMultiselected(this->itemsMenu->GetSelectedItem(), true);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void MenuLayout::menu_OnSelected(u32 index)
    {
        this->itemAuthor->SetVisible(true);
        this->itemVersion->SetVisible(true);
        u32 realidx = index;
        if(this->homebrew_mode)
        {
            if(index == 0)
            {
                this->itemAuthor->SetVisible(false);
                this->itemVersion->SetVisible(false);
                this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerHomebrew.png"));
                this->itemName->SetText(cfg::GetLanguageString(config.main_lang, config.default_lang, "hbmenu_launch"));
            }
            else
            {
                realidx--;
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
                    this->itemName->SetText(cfg::GetLanguageString(config.main_lang, config.default_lang, "unknown"));
                    this->itemAuthor->SetText(cfg::GetLanguageString(config.main_lang, config.default_lang, "unknown"));
                }
                if(strlen(info.nacp.version)) this->itemVersion->SetText(info.nacp.version);
                else this->itemVersion->SetText("0");
                this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerHomebrew.png"));
            }
        }
        else
        {
            auto &folder = cfg::FindFolderByName(list, this->curfolder);
            s32 titleidx = realidx;
            if(this->curfolder.empty())
            {
                if(realidx >= list.folders.size())
                {
                    titleidx -= list.folders.size();
                }
                else
                {
                    auto foldr = list.folders[realidx];
                    this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerFolder.png"));
                    auto sz = foldr.titles.size();
                    this->itemAuthor->SetText(std::to_string(sz) + " " + ((sz == 1) ? cfg::GetLanguageString(config.main_lang, config.default_lang, "folder_entry_single") : cfg::GetLanguageString(config.main_lang, config.default_lang, "folder_entry_mult")));
                    this->itemVersion->SetVisible(false);
                    this->itemName->SetText(foldr.name);
                    titleidx = -1;
                }
                
            }
            if(titleidx >= 0)
            {
                auto title = folder.titles[titleidx];
                auto info = cfg::GetRecordInformation(title);
                auto lent = cfg::GetRecordInformationLanguageEntry(info);
                if(lent != NULL)
                {
                    this->itemName->SetText(lent->name);
                    this->itemAuthor->SetText(lent->author);
                }
                else
                {
                    this->itemName->SetText(cfg::GetLanguageString(config.main_lang, config.default_lang, "unknown"));
                    this->itemAuthor->SetText(cfg::GetLanguageString(config.main_lang, config.default_lang, "unknown"));
                }
                if(strlen(info.nacp.version)) this->itemVersion->SetText(info.nacp.version);
                else this->itemVersion->SetText("0");
                if((cfg::TitleType)title.title_type == cfg::TitleType::Homebrew) this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerHomebrew.png"));
                else this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerInstalled.png"));
            }
        }
        if(!this->curfolder.empty()) this->bannerImage->SetImage(cfg::ProcessedThemeResource(theme, "ui/BannerFolder.png")); // This way user always knows he's inside a folder
    }

    void MenuLayout::MoveFolder(std::string name, bool fade)
    {
        if(fade) qapp->FadeOut();

        if(this->homebrew_mode)
        {
            if(homebrew.empty()) homebrew = cfg::QueryAllHomebrew();
        }

        auto itm_list = homebrew;
        if(!this->homebrew_mode)
        {
            auto &folder = cfg::FindFolderByName(list, name);
            itm_list = folder.titles;
        }

        this->itemsMenu->ClearItems();
        if(this->homebrew_mode) this->itemsMenu->AddItem(cfg::ProcessedThemeResource(theme, "ui/Hbmenu.png"));
        else
        {
            if(name.empty())
            {
                STL_REMOVE_IF(list.folders, fldr, (fldr.titles.empty())) // Remove empty folders
                for(auto folder: list.folders) this->itemsMenu->AddItem(cfg::ProcessedThemeResource(theme, "ui/Folder.png"));
            }
        }

        u32 tmpidx = 0;
        for(auto itm: itm_list)
        {
            bool set_susp = false;
            if((cfg::TitleType)itm.title_type == cfg::TitleType::Installed)
            {
                if(qapp->IsTitleSuspended()) if(qapp->GetSuspendedApplicationId() == itm.app_id) set_susp = true;
            }
            else
            {
                if(qapp->IsHomebrewSuspended()) if(qapp->GetSuspendedHomebrewPath() == std::string(itm.nro_target.nro_path)) set_susp = true;
            }
            this->itemsMenu->AddItem(cfg::GetRecordIconPath(itm));
            if(set_susp)
            {
                u32 suspidx = tmpidx;
                if(this->homebrew_mode) suspidx++; // Skip initial item if homebrew mode
                else if(name.empty()) suspidx += list.folders.size(); // Ignore front folders from main menu
                this->itemsMenu->SetSuspendedItem(suspidx);
            }
            tmpidx++;
        }

        this->itemsMenu->UpdateBorderIcons();
        if(!this->homebrew_mode) this->curfolder = name;

        if(fade) qapp->FadeIn();
    }

    void MenuLayout::OnInput(u64 down, u64 up, u64 held, pu::ui::Touch pos)
    {
        bool hasconn = net::HasConnection();
        if(this->last_hasconn != hasconn)
        {
            if(hasconn) this->connIcon->SetImage(cfg::ProcessedThemeResource(theme, "ui/ConnectionIcon.png"));
            else this->connIcon->SetImage(cfg::ProcessedThemeResource(theme, "ui/NoConnectionIcon.png"));
            this->last_hasconn = hasconn;
        }

        auto curtime = util::GetCurrentTime();
        this->timeText->SetText(curtime);

        auto lvl = util::GetBatteryLevel();
        if(this->last_batterylvl != lvl)
        {
            this->last_batterylvl = lvl;
            auto lvlstr = std::to_string(lvl) + "%";
            this->batteryText->SetText(lvlstr);
        }
        bool ch = util::IsCharging();
        if(this->last_charge != ch)
        {
            this->last_charge = ch;
            if(ch) this->batteryIcon->SetImage(cfg::ProcessedThemeResource(theme, "ui/BatteryChargingIcon.png"));
            else this->batteryIcon->SetImage(cfg::ProcessedThemeResource(theme, "ui/BatteryNormalIcon.png"));
        }

        auto ctp = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(ctp - this->tp).count() >= 500)
        {
            if(qapp->LaunchFailed() && !this->warnshown)
            {
                qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "app_launch"), cfg::GetLanguageString(config.main_lang, config.default_lang, "app_launch_post_error"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "ok") }, true);
                this->warnshown = true;
            }
        }

        if(am::QMenuIsHomePressed())
        {
            if(qapp->IsSuspended())
            {
                if(this->mode == 1) this->mode = 2;
            }
            else while(this->itemsMenu->GetSelectedItem() > 0) this->itemsMenu->HandleMoveLeft();
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
        else if(down & KEY_PLUS) this->logo_Click();
        else if(down & KEY_MINUS) this->menuToggle_Click();
        else if(down & KEY_ZL) this->HandleUserMenu();
        else if(down & KEY_L) this->HandleWebPageOpen();
        else if(down & KEY_R) this->HandleSettingsMenu();
        else if(down & KEY_ZR) this->HandleThemesMenu();
    }

    void MenuLayout::SetUser(u128 user)
    {
        this->users->SetImage(os::GetIconCacheImagePath(user));
        this->users->SetWidth(50);
        this->users->SetHeight(50);
    }

    void MenuLayout::menuToggle_Click()
    {
        pu::audio::Play(this->sfxMenuToggle);
        this->homebrew_mode = !this->homebrew_mode;
        if(this->select_on)
        {
            this->select_on = false;
            this->itemsMenu->ResetMultiselections();
        }
        this->MoveFolder("", true);
    }

    void MenuLayout::logo_Click()
    {
        qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "ulaunch_about"), "uLaunch v" + std::string(Q_VERSION) + "\n\n" + cfg::GetLanguageString(config.main_lang, config.default_lang, "ulaunch_about") + "\n\n" + cfg::GetLanguageString(config.main_lang, config.default_lang, "ulaunch_contribute") + ":\nhttps://github.com/XorTroll/uLaunch", { cfg::GetLanguageString(config.main_lang, config.default_lang, "ok") }, true, "romfs:/LogoLarge.png");
        qapp->ShowNotification("(-) -> " + cfg::GetLanguageString(config.main_lang, config.default_lang, "control_minus") + "  |  (X) -> " + cfg::GetLanguageString(config.main_lang, config.default_lang, "control_x") + " | (Y) -> " + cfg::GetLanguageString(config.main_lang, config.default_lang, "control_y") + " | (L), (R), (ZL), (ZR) -> " + cfg::GetLanguageString(config.main_lang, config.default_lang, "control_zlr"), 3500);
    }

    void MenuLayout::settings_Click()
    {
        this->HandleSettingsMenu();
    }

    void MenuLayout::themes_Click()
    {
        this->HandleThemesMenu();
    }

    void MenuLayout::users_Click()
    {
        this->HandleUserMenu();
    }

    void MenuLayout::web_Click()
    {
        this->HandleWebPageOpen();
    }

    void MenuLayout::HandleCloseSuspended()
    {
        auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "suspended_app"), cfg::GetLanguageString(config.main_lang, config.default_lang, "suspended_close"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "no") }, true);
        if(sopt == 0)
        {
            am::QMenuCommandWriter writer(am::QDaemonMessage::TerminateApplication);
            writer.FinishWrite();

            this->itemsMenu->UnsetSuspendedItem();
            qapp->NotifyEndSuspended();
            this->bgSuspendedRaw->SetAlphaFactor(0);
        }
    }

    void MenuLayout::HandleHomebrewLaunch(cfg::TitleRecord &rec)
    {
        u32 launchmode = 0;
        if(config.system_title_override_enabled)
        {
            auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "hb_launch"), cfg::GetLanguageString(config.main_lang, config.default_lang, "hb_launch_conf"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "hb_applet"), cfg::GetLanguageString(config.main_lang, config.default_lang, "hb_app"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
            if(sopt == 0) launchmode = 1;
            else if(sopt == 1) launchmode = 2;
        }
        else launchmode = 1;
        if(launchmode == 1)
        {
            hb::TargetInput ipt = {};
            strcpy(ipt.nro_path, rec.nro_target.nro_path);
            strcpy(ipt.argv, rec.nro_target.nro_path);
            if(strlen(rec.nro_target.argv)) sprintf(ipt.argv, "%s %s", rec.nro_target.nro_path, rec.nro_target.argv);

            am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchHomebrewLibApplet);
            writer.Write<hb::TargetInput>(ipt);
            writer.FinishWrite();

            pu::audio::Play(this->sfxTitleLaunch);
            qapp->StopPlayBGM();
            qapp->CloseWithFadeOut();
            return;
        }
        else if(launchmode == 2)
        {
            bool launch = true;
            if(qapp->IsSuspended())
            {
                launch = false;
                this->HandleCloseSuspended();
                if(!qapp->IsSuspended()) launch = true;
            }
            if(launch)
            {
                hb::TargetInput ipt = {};
                strcpy(ipt.nro_path, rec.nro_target.nro_path);
                strcpy(ipt.argv, rec.nro_target.nro_path);
                if(strlen(rec.nro_target.argv)) sprintf(ipt.argv, "%s %s", rec.nro_target.nro_path, rec.nro_target.argv);

                am::QMenuCommandWriter writer(am::QDaemonMessage::LaunchHomebrewApplication);
                writer.Write<hb::TargetInput>(ipt);
                writer.FinishWrite();

                am::QMenuCommandResultReader reader;
                if(reader && R_SUCCEEDED(reader.GetReadResult()))
                {
                    pu::audio::Play(this->sfxTitleLaunch);
                    qapp->StopPlayBGM();
                    qapp->CloseWithFadeOut();
                    return;
                }
                else
                {
                    auto rc = reader.GetReadResult();
                    qapp->ShowNotification(cfg::GetLanguageString(config.main_lang, config.default_lang, "app_launch_error") + ": " + util::FormatResult(rc));
                }
                reader.FinishRead();
            }
        }
    }

    void MenuLayout::HandleUserMenu()
    {
        auto uid = qapp->GetSelectedUser();
        
        am::QMenuCommandWriter writer(am::QDaemonMessage::UserHasPassword);
        writer.Write<u128>(uid);
        writer.FinishWrite();

        am::QMenuCommandResultReader res;
        res.FinishRead();

        bool has_pass = R_SUCCEEDED(res.GetReadResult());

        auto [_rc, name] = os::GetAccountName(uid);
        auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "user_settings"), cfg::GetLanguageString(config.main_lang, config.default_lang, "user_selected") + ": " + name + "\n" + cfg::GetLanguageString(config.main_lang, config.default_lang, "user_option"), { has_pass ? cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_ch") : cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_reg"), cfg::GetLanguageString(config.main_lang, config.default_lang, "user_view_page"), cfg::GetLanguageString(config.main_lang, config.default_lang, "user_logoff"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true, os::GetIconCacheImagePath(uid));
        if(sopt == 0)
        {
            if(has_pass)
            {
                auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_ch"), cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_ch_option"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_change"), cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_remove"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
                if((sopt == 0) || (sopt == 1))
                {
                    SwkbdConfig swkbd;
                    swkbdCreate(&swkbd, 0);
                    swkbdConfigMakePresetPassword(&swkbd);
                    swkbdConfigSetStringLenMax(&swkbd, 15);
                    swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(config.main_lang, config.default_lang, "swkbd_user_pass_guide").c_str());
                    char inpass[0x10] = {0};
                    auto rc = swkbdShow(&swkbd, inpass, 0x10);
                    swkbdClose(&swkbd);
                    if(R_SUCCEEDED(rc))
                    {
                        auto [rc2, oldpass] = db::PackPassword(uid, inpass);
                        rc = rc2;
                        if(R_SUCCEEDED(rc))
                        {
                            if(sopt == 0)
                            {
                                SwkbdConfig swkbd;
                                swkbdCreate(&swkbd, 0);
                                swkbdConfigMakePresetPassword(&swkbd);
                                swkbdConfigSetStringLenMax(&swkbd, 15);
                                swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(config.main_lang, config.default_lang, "swkbd_user_new_pass_guide").c_str());
                                char pass[0x10] = {0};
                                auto rc = swkbdShow(&swkbd, pass, 0x10);
                                swkbdClose(&swkbd);
                                if(R_SUCCEEDED(rc))
                                {
                                    auto sopt2 = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_ch"), cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_change_conf"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
                                    if(sopt2 == 0)
                                    {
                                        auto [rc3, newpass] = db::PackPassword(uid, pass);
                                        rc = rc3;
                                        if(R_SUCCEEDED(rc))
                                        {
                                            am::QMenuCommandWriter writer(am::QDaemonMessage::ChangeUserPassword);
                                            writer.Write<db::PassBlock>(oldpass);
                                            writer.Write<db::PassBlock>(newpass);
                                            writer.FinishWrite();

                                            am::QMenuCommandResultReader reader;
                                            rc = reader.GetReadResult();
                                        }
                                        qapp->ShowNotification(R_SUCCEEDED(rc) ? cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_change_ok") : cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_change_error") + ": " + util::FormatResult(rc));
                                    }
                                }
                            }
                            else if(sopt == 1)
                            {
                                auto sopt2 = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_remove_full"), cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_remove_conf"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
                                if(sopt2 == 0)
                                {
                                    am::QMenuCommandWriter writer(am::QDaemonMessage::RemoveUserPassword);
                                    writer.Write<db::PassBlock>(oldpass);
                                    writer.FinishWrite();

                                    am::QMenuCommandResultReader reader;
                                    rc = reader.GetReadResult();

                                    qapp->ShowNotification(R_SUCCEEDED(rc) ? cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_remove_ok") : cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_remove_error") + ": " + util::FormatResult(rc));
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                SwkbdConfig swkbd;
                swkbdCreate(&swkbd, 0);
                swkbdConfigMakePresetPassword(&swkbd);
                swkbdConfigSetStringLenMax(&swkbd, 15);
                swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(config.main_lang, config.default_lang, "swkbd_user_pass_guide").c_str());
                char pass[0x10] = {0};
                auto rc = swkbdShow(&swkbd, pass, 0x10);
                swkbdClose(&swkbd);
                if(R_SUCCEEDED(rc))
                {
                    auto [rc2, newpass] = db::PackPassword(uid, pass);
                    rc = rc2;
                    if(R_SUCCEEDED(rc))
                    {
                        auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_reg"), cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_reg_conf"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
                        if(sopt == 0)
                        {
                            am::QMenuCommandWriter writer(am::QDaemonMessage::RegisterUserPassword);
                            writer.Write<db::PassBlock>(newpass);
                            writer.FinishWrite();

                            am::QMenuCommandResultReader reader;
                            rc = reader.GetReadResult();
                        }
                    }
                    qapp->ShowNotification(R_SUCCEEDED(rc) ? cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_reg_ok") : cfg::GetLanguageString(config.main_lang, config.default_lang, "user_pass_reg_error") + ": " + util::FormatResult(rc));
                }
            }
        }
        else if(sopt == 1)
        {
            // Show myPage applet for user config
            u8 in[0xb0] = {0};
            *(u32*)in = 7; // Type -> ShowMyProfile
            memcpy((u128*)(in + 0x8), &uid, sizeof(uid));

            am::LibraryAppletQMenuLaunchAnd(AppletId_myPage, 1, in, sizeof(in), NULL, 0, [&]() -> bool
            {
                return !am::QMenuIsHomePressed();
            });
        }
        else if(sopt == 2)
        {
            u32 logoff = 0;
            if(qapp->IsSuspended())
            {
                auto sopt = qapp->CreateShowDialog(cfg::GetLanguageString(config.main_lang, config.default_lang, "suspended_app"), cfg::GetLanguageString(config.main_lang, config.default_lang, "user_logoff_app_suspended"), { cfg::GetLanguageString(config.main_lang, config.default_lang, "yes"), cfg::GetLanguageString(config.main_lang, config.default_lang, "cancel") }, true);
                if(sopt == 0) logoff = 2;
            }
            else logoff = 1;

            if(logoff > 0)
            {
                if(logoff == 2)
                {
                    am::QMenuCommandWriter writer(am::QDaemonMessage::TerminateApplication);
                    writer.FinishWrite();

                    this->itemsMenu->UnsetSuspendedItem();
                    qapp->NotifyEndSuspended();
                    this->bgSuspendedRaw->SetAlphaFactor(0);
                }
                qapp->FadeOut();
                this->MoveFolder("", false);
                qapp->LoadStartupMenu();
                qapp->FadeIn();
            }
        }
    }

    void MenuLayout::HandleWebPageOpen()
    {
        SwkbdConfig swkbd;
        swkbdCreate(&swkbd, 0);
        swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(config.main_lang, config.default_lang, "swkbd_webpage_guide").c_str());
        char url[500] = {0};
        auto rc = swkbdShow(&swkbd, url, 500);
        swkbdClose(&swkbd);
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
    
    void MenuLayout::HandleSettingsMenu()
    {
        qapp->FadeOut();
        qapp->LoadSettingsMenu();
        qapp->FadeIn();
    }
    
    void MenuLayout::HandleThemesMenu()
    {
        qapp->FadeOut();
        qapp->LoadThemeMenu();
        qapp->FadeIn();
    }
}