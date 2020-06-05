#include <ui/ui_MenuLayout.hpp>
#include <os/os_Titles.hpp>
#include <os/os_Account.hpp>
#include <util/util_Convert.hpp>
#include <util/util_Misc.hpp>
#include <os/os_Misc.hpp>
#include <am/am_LibraryApplet.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <os/os_HomeMenu.hpp>
#include <fs/fs_Stdio.hpp>
#include <net/net_Service.hpp>

extern ui::MenuApplication::Ref g_menu_app_instance;
extern cfg::TitleList g_entry_list;
extern std::vector<cfg::TitleRecord> g_homebrew_records;
extern cfg::Config g_ul_config;
extern cfg::Theme g_ul_theme;

#define MENU_HBMENU_NRO "sdmc:/hbmenu.nro"

namespace ui {

    MenuLayout::MenuLayout(void *raw, u8 min_alpha) : susptr(raw), last_hasconn(false), last_batterylvl(0), last_charge(false), warnshown(false), homebrew_mode(false), select_on(false), select_dir(false), minalpha(min_alpha), mode(0), rawalpha(0xFF) {
        auto textclr = pu::ui::Color::FromHex(g_menu_app_instance->GetUIConfigValue<std::string>("text_color", "#e1e1e1ff"));
        auto menutextx = g_menu_app_instance->GetUIConfigValue<u32>("menu_folder_text_x", 30);
        auto menutexty = g_menu_app_instance->GetUIConfigValue<u32>("menu_folder_text_y", 200);
        auto menutextsz = g_menu_app_instance->GetUIConfigValue<u32>("menu_folder_text_size", 25);

        this->bgSuspendedRaw = RawData::New(0, 0, raw, 1280, 720, 4);
        this->Add(this->bgSuspendedRaw);

        // Load banners first
        this->topMenuImage = pu::ui::elm::Image::New(40, 35, cfg::GetAssetByTheme(g_ul_theme, "ui/TopMenu.png"));
        g_menu_app_instance->ApplyConfigForElement("main_menu", "top_menu_bg", this->topMenuImage);
        this->Add(this->topMenuImage);

        this->bannerImage = pu::ui::elm::Image::New(0, 585, cfg::GetAssetByTheme(g_ul_theme, "ui/BannerInstalled.png"));
        g_menu_app_instance->ApplyConfigForElement("main_menu", "banner_image", this->bannerImage);
        this->Add(this->bannerImage);

        // Then load buttons and other UI elements
        this->logo = ClickableImage::New(610, 13 + 35, "romfs:/Logo.png");
        this->logo->SetWidth(60);
        this->logo->SetHeight(60);
        this->logo->SetOnClick(&actions::ShowAboutDialog);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "logo_icon", this->logo, false); // Sorry theme makers... logo must be visible, but can be moved
        this->Add(this->logo);

        this->connIcon = pu::ui::elm::Image::New(80, 53, cfg::GetAssetByTheme(g_ul_theme, "ui/NoConnectionIcon.png"));
        g_menu_app_instance->ApplyConfigForElement("main_menu", "connection_icon", this->connIcon);
        this->Add(this->connIcon);
        this->users = ClickableImage::New(270, 53, ""); // On layout creation, no user is still selected...
        this->users->SetOnClick(&actions::ShowUserMenu);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "user_icon", this->users);
        this->Add(this->users);
        this->controller = ClickableImage::New(340, 53, cfg::GetAssetByTheme(g_ul_theme, "ui/ControllerIcon.png"));
        this->controller->SetOnClick(&actions::ShowControllerSupport);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "controller_icon", this->controller);
        this->Add(this->controller);

        auto curtime = os::GetCurrentTime();
        this->timeText = pu::ui::elm::TextBlock::New(515, 68, curtime);
        this->timeText->SetColor(textclr);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "time_text", this->timeText);
        this->Add(this->timeText);
        auto lvl = os::GetBatteryLevel();
        auto lvlstr = std::to_string(lvl) + "%";
        this->batteryText = pu::ui::elm::TextBlock::New(700, 55, lvlstr);
        this->batteryText->SetColor(textclr);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "battery_text", this->batteryText);
        this->Add(this->batteryText);
        this->batteryIcon = pu::ui::elm::Image::New(700, 80, cfg::GetAssetByTheme(g_ul_theme, "ui/BatteryNormalIcon.png"));
        g_menu_app_instance->ApplyConfigForElement("main_menu", "battery_icon", this->batteryIcon);
        this->Add(this->batteryIcon);

        this->settings = ClickableImage::New(880, 53, cfg::GetAssetByTheme(g_ul_theme, "ui/SettingsIcon.png"));
        this->settings->SetOnClick(&actions::ShowSettingsMenu);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "settings_icon", this->settings);
        this->Add(this->settings);
        this->themes = ClickableImage::New(950, 53, cfg::GetAssetByTheme(g_ul_theme, "ui/ThemesIcon.png"));
        this->themes->SetOnClick(&actions::ShowThemesMenu);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "themes_icon", this->themes);
        this->Add(this->themes);

        this->fwText = pu::ui::elm::TextBlock::New(1140, 68, os::GetFirmwareVersion());
        this->fwText->SetColor(textclr);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "firmware_text", this->fwText);
        this->Add(this->fwText);

        this->menuToggle = ClickableImage::New(520, 200, cfg::GetAssetByTheme(g_ul_theme, "ui/ToggleClick.png"));
        this->menuToggle->SetOnClick(std::bind(&MenuLayout::menuToggle_Click, this));
        g_menu_app_instance->ApplyConfigForElement("main_menu", "menu_toggle_button", this->menuToggle);
        this->Add(this->menuToggle);

        this->itemName = pu::ui::elm::TextBlock::New(40, 610, "A");
        this->itemName->SetFont("DefaultFont@30");
        this->itemName->SetColor(textclr);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "banner_name_text", this->itemName);
        this->Add(this->itemName);
        this->itemAuthor = pu::ui::elm::TextBlock::New(45, 650, "A");
        this->itemAuthor->SetFont("DefaultFont@20");
        this->itemAuthor->SetColor(textclr);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "banner_author_text", this->itemAuthor);
        this->Add(this->itemAuthor);
        this->itemVersion = pu::ui::elm::TextBlock::New(45, 675, "A");
        this->itemVersion->SetFont("DefaultFont@20");
        this->itemVersion->SetColor(textclr);
        g_menu_app_instance->ApplyConfigForElement("main_menu", "banner_version_text", this->itemVersion);
        this->Add(this->itemVersion);
        std::string font_name = "DefaultFont@" + std::to_string(menutextsz);
        this->itemsMenu = SideMenu::New(pu::ui::Color(0, 255, 120, 0xFF), cfg::GetAssetByTheme(g_ul_theme, "ui/Cursor.png"), cfg::GetAssetByTheme(g_ul_theme, "ui/Suspended.png"), cfg::GetAssetByTheme(g_ul_theme, "ui/Multiselect.png"), menutextx, menutexty, font_name, textclr, 294);
        this->MoveFolder("", false);
        this->itemsMenu->SetOnItemSelected(std::bind(&MenuLayout::menu_Click, this, std::placeholders::_1, std::placeholders::_2));
        this->itemsMenu->SetOnSelectionChanged(std::bind(&MenuLayout::menu_OnSelected, this, std::placeholders::_1));
        g_menu_app_instance->ApplyConfigForElement("main_menu", "items_menu", this->itemsMenu, false); // Main menu must be visible, and only Y is customizable here
        this->Add(this->itemsMenu);

        this->quickMenu = QuickMenu::New(cfg::GetAssetByTheme(g_ul_theme, "ui/QuickMenuMain.png"));
        this->Add(this->quickMenu);

        this->tp = std::chrono::steady_clock::now();

        this->sfxTitleLaunch = pu::audio::Load(cfg::GetAssetByTheme(g_ul_theme, "sound/TitleLaunch.wav"));
        this->sfxMenuToggle = pu::audio::Load(cfg::GetAssetByTheme(g_ul_theme, "sound/MenuToggle.wav"));

        this->SetBackgroundImage(cfg::GetAssetByTheme(g_ul_theme, "ui/Background.png"));
    }

    MenuLayout::~MenuLayout() {
        pu::audio::DeleteSfx(this->sfxTitleLaunch);
        pu::audio::DeleteSfx(this->sfxMenuToggle);
    }

    void MenuLayout::OnMenuInput(u64 down, u64 up, u64 held, pu::ui::Touch touch_pos) {
        auto quickon = this->quickMenu->IsOn();
        this->itemsMenu->SetEnabled(!quickon);
        if(quickon) {
            return;
        }

        auto hasconn = net::HasConnection();
        if(this->last_hasconn != hasconn) {
            this->last_hasconn = hasconn;
            std::string connection_img = "ui/NoConnectionIcon.png";
            if(hasconn) {
                connection_img = "ui/ConnectionIcon.png";
            }
            this->connIcon->SetImage(cfg::GetAssetByTheme(g_ul_theme, connection_img));
        }

        auto curtime = os::GetCurrentTime();
        this->timeText->SetText(curtime);

        auto lvl = os::GetBatteryLevel();
        if(this->last_batterylvl != lvl) {
            this->last_batterylvl = lvl;
            auto lvlstr = std::to_string(lvl) + "%";
            this->batteryText->SetText(lvlstr);
        }

        auto ch = os::IsConsoleCharging();
        if(this->last_charge != ch) {
            this->last_charge = ch;
            std::string battery_img = "ui/BatteryNormalIcon.png";
            if(ch) {
                battery_img = "ui/BatteryChargingIcon.png";
            }
            this->batteryIcon->SetImage(cfg::GetAssetByTheme(g_ul_theme, battery_img));
        }

        auto ctp = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(ctp - this->tp).count() >= 500) {
            if(g_menu_app_instance->LaunchFailed() && !this->warnshown) {
                g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "app_launch"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "app_unexpected_error"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "ok") }, true);
                this->warnshown = true;
            }
        }

        if(this->susptr != nullptr) {
            if(this->mode == 0) {
                if(this->rawalpha == this->minalpha) {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    this->mode = 1;
                }
                else {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    this->rawalpha -= 10;
                    if(this->rawalpha < this->minalpha) {
                        this->rawalpha = this->minalpha;
                    }
                    else {
                        this->ApplySuspendedRatio(false);
                    }
                }
            }
            else if(this->mode == 2) {
                if(this->rawalpha == 0xFF) {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    dmi::MenuMessageWriter writer(dmi::DaemonMessage::ResumeApplication);
                    writer.FinishWrite();

                    dmi::MenuResultReader reader;
                    reader.FinishRead();
                }
                else {
                    this->bgSuspendedRaw->SetAlphaFactor(this->rawalpha);
                    this->rawalpha += 10;
                    if(this->rawalpha > 0xFF) {
                        this->rawalpha = 0xFF;
                    }
                    else {
                        this->ApplySuspendedRatio(true);
                    }
                }
            }
        }

        if(down & KEY_B) {
            if(!this->curfolder.empty() && !this->homebrew_mode) {
                this->MoveFolder("", true);
            }
        }
        else if(down & KEY_PLUS) {
            actions::ShowAboutDialog();
        }
        else if(down & KEY_MINUS) {
            this->menuToggle_Click();
        }
    }

    void MenuLayout::OnHomeButtonPress()
    {
        if(g_menu_app_instance->IsSuspended()) {
            if(this->mode == 1) {
                this->mode = 2;
            }
        }
        else {
            while(this->itemsMenu->GetSelectedItem() > 0) {
                // TODO: improve speed of this movement (when too far, might be quite annoying)
                this->itemsMenu->HandleMoveLeft();
            }
        }
    }

    void MenuLayout::menu_Click(u64 down, u32 index) {
        if((this->select_on) && (down & KEY_A)) {
            if(!this->itemsMenu->IsAnyMultiselected()) {
                this->StopMultiselect();
            }
        }
        if(this->select_on) {
            if(select_dir) {
                if((down & KEY_A) || (down & KEY_Y)) {
                    if((!this->homebrew_mode) && this->curfolder.empty()) {
                        if(index < g_entry_list.folders.size()) {
                            auto &folder = g_entry_list.folders[index];
                            auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_multiselect"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_move_existing_folder_conf"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "yes"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "no"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "cancel") }, true);
                            if(sopt == 0) {
                                this->HandleMultiselectMoveToFolder(folder.name);
                            }
                            else if(sopt == 1) {
                                this->StopMultiselect();
                            }
                        }
                    }
                }
                else if(down & KEY_B) {
                    this->select_dir = false;
                    g_menu_app_instance->ShowNotification(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_move_select_folder_cancel"));
                }
            }
            else {
                if(down & KEY_B) {
                    g_menu_app_instance->ShowNotification(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_multiselect_cancel"));
                    this->StopMultiselect();
                }
                else if(down & KEY_Y) {
                    auto selectable = false;
                    if(this->homebrew_mode) {
                        selectable = true;
                    }
                    else {
                        if(this->curfolder.empty()) {
                            selectable = index >= g_entry_list.folders.size();
                        }
                        else {
                            selectable = true;
                        }
                    }
                    if(selectable) {
                        this->itemsMenu->SetItemMultiselected(index, !this->itemsMenu->IsItemMultiselected(index));
                    }
                }
                else if(down & KEY_A) {
                    if(this->homebrew_mode) {
                        auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_multiselect"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "hb_mode_entries_add"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "yes"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "no"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "cancel") }, true);
                        if(sopt == 0) {
                            // Get the idx of the last g_homebrew_records element.
                            s32 hbidx = 0;
                            for(auto &entry: g_entry_list.root.titles) {
                                if(static_cast<cfg::TitleType>(entry.title_type) == cfg::TitleType::Installed) {
                                    break;
                                }
                                hbidx++;
                            }
                            if(hbidx < 0) {
                                hbidx = 0;
                            }
                            auto any = false;
                            for(u32 i = 0; i < g_homebrew_records.size(); i++) {
                                auto &hb = g_homebrew_records[i];
                                auto idx = i + 1;
                                if(this->itemsMenu->IsItemMultiselected(idx)) {
                                    if(!cfg::ExistsRecord(g_entry_list, hb)) {
                                        cfg::SaveRecord(hb);
                                        g_entry_list.root.titles.insert(g_entry_list.root.titles.begin() + hbidx, hb);
                                        any = true;
                                        hbidx++;
                                    }
                                }
                            }
                            if(any) {
                                g_menu_app_instance->ShowNotification(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "hb_mode_entries_added"));
                            }
                            this->StopMultiselect();
                        }
                        else if(sopt == 1) {
                            this->StopMultiselect();
                        }
                    }
                    else if(this->curfolder.empty()) {
                        auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_multiselect"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_move_to_folder"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_move_new_folder"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_move_existing_folder"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "no"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "cancel") }, true);
                        if(sopt == 0) {
                            SwkbdConfig swkbd;
                            swkbdCreate(&swkbd, 0);
                            swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "swkbd_new_folder_guide").c_str());
                            char dir[500] = {0};
                            auto rc = swkbdShow(&swkbd, dir, 500);
                            swkbdClose(&swkbd);
                            if(R_SUCCEEDED(rc)) {
                                this->HandleMultiselectMoveToFolder(dir);
                            }
                        }
                        else if(sopt == 1) {
                            this->select_dir = true;
                            g_menu_app_instance->ShowNotification(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_move_select_folder"));
                        }
                        else if(sopt == 2) {
                            this->StopMultiselect();
                        }
                    }
                    else {
                        auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_multiselect"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_move_from_folder"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "yes"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "no"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "cancel") }, true);
                        if(sopt == 0) {
                            u32 rmvd = 0;
                            auto &folder = cfg::FindFolderByName(g_entry_list, this->curfolder);
                            auto basesz = folder.titles.size();
                            for(u32 i = 0; i < basesz; i++) {
                                auto &title = folder.titles[i - rmvd];
                                if(this->itemsMenu->IsItemMultiselected(i)) {
                                    if(cfg::MoveRecordTo(g_entry_list, title, "")) {
                                        rmvd++;
                                    }
                                }
                            }
                            this->StopMultiselect();
                            this->MoveFolder(folder.titles.empty() ? "" : this->curfolder, true);
                        }
                        else if(sopt == 1) {
                            this->StopMultiselect();
                        }
                    }
                }
            }
        }
        else {
            if((index == 0) && this->homebrew_mode) {
                if(down & KEY_A) {
                    pu::audio::Play(this->sfxTitleLaunch);

                    dmi::MenuMessageWriter writer(dmi::DaemonMessage::LaunchHomebrewLibraryApplet);
                    hb::HbTargetParams ipt = {};
                    // Launch normal hbmenu
                    strcpy(ipt.nro_path, MENU_HBMENU_NRO);
                    strcpy(ipt.nro_argv, MENU_HBMENU_NRO);
                    writer.Write<hb::HbTargetParams>(ipt);

                    g_menu_app_instance->StopPlayBGM();
                    g_menu_app_instance->CloseWithFadeOut();
                    return;
                }
            }
            else {
                s32 realidx = index;
                if(this->homebrew_mode) {
                    realidx--;
                    auto hb = g_homebrew_records[realidx];
                    if(down & KEY_A) {
                        auto hblaunch = true;
                        if(g_menu_app_instance->IsHomebrewSuspended()) {
                            if(g_menu_app_instance->EqualsSuspendedHomebrewPath(hb.nro_target.nro_path)) {
                                if(this->mode == 1) {
                                    this->mode = 2;
                                }
                                hblaunch = false;
                            }
                            else {
                                hblaunch = false;
                                this->HandleCloseSuspended();
                                hblaunch = !g_menu_app_instance->IsHomebrewSuspended();
                            }
                        }
                        if(hblaunch) {
                            this->HandleHomebrewLaunch(hb);
                        }
                    }
                    else if(down & KEY_X) {
                        if(g_menu_app_instance->IsSuspended()) {
                            if(g_menu_app_instance->EqualsSuspendedHomebrewPath(hb.nro_target.nro_path)) {
                                this->HandleCloseSuspended();
                            }
                        }
                    }
                    else if(down & KEY_Y) {
                        this->select_on = true;
                        this->itemsMenu->SetItemMultiselected(this->itemsMenu->GetSelectedItem(), true);
                    }
                }
                else {
                    auto &folder = cfg::FindFolderByName(g_entry_list, this->curfolder);
                    s32 titleidx = realidx;
                    if(this->curfolder.empty()) {
                        if(realidx >= g_entry_list.folders.size()) {
                            titleidx -= g_entry_list.folders.size();
                        }
                        else {
                            auto foldr = g_entry_list.folders[realidx];
                            if(down & KEY_A) {
                                this->MoveFolder(foldr.name, true);
                            }
                            else if(down & KEY_Y) {
                                auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_rename_folder"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_rename_folder_conf"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "yes"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "no") }, true);
                                if(sopt == 0) {
                                    SwkbdConfig swkbd;
                                    swkbdCreate(&swkbd, 0);
                                    swkbdConfigSetGuideText(&swkbd, cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "swkbd_rename_folder_guide").c_str());
                                    char dir[500] = {0};
                                    auto rc = swkbdShow(&swkbd, dir, 500);
                                    swkbdClose(&swkbd);
                                    if(R_SUCCEEDED(rc)) {
                                        cfg::RenameFolder(g_entry_list, foldr.name, dir);
                                        this->MoveFolder(this->curfolder, true);
                                    }
                                }
                            }
                            titleidx = -1;
                        }
                    }
                    if(titleidx >= 0) {
                        auto title = folder.titles[titleidx];
                        const auto type = static_cast<cfg::TitleType>(title.title_type);
                        if(down & KEY_A) {
                            bool titlelaunch = true;

                            if(g_menu_app_instance->IsSuspended()) {
                                if(type == cfg::TitleType::Homebrew) {
                                    if(g_menu_app_instance->EqualsSuspendedHomebrewPath(title.nro_target.nro_path)) {
                                        if(this->mode == 1) {
                                            this->mode = 2;
                                        }
                                        titlelaunch = false;
                                    }
                                }
                                else if(type == cfg::TitleType::Installed) {
                                    if(title.app_id == g_menu_app_instance->GetSuspendedApplicationId()) {
                                        if(this->mode == 1) {
                                            this->mode = 2;
                                        }
                                        titlelaunch = false;
                                    }
                                }
                                if(titlelaunch) {
                                    // Homebrew launching code already does this checks later - doing this check only with installed titles
                                    if(type == cfg::TitleType::Installed) {
                                        titlelaunch = false;
                                        this->HandleCloseSuspended();
                                        titlelaunch = !g_menu_app_instance->IsSuspended();
                                    }
                                }
                            }
                            if(titlelaunch) {
                                if(type == cfg::TitleType::Homebrew) {
                                    this->HandleHomebrewLaunch(title);
                                }
                                else {
                                    pu::audio::Play(this->sfxTitleLaunch);
                                    {
                                        dmi::MenuMessageWriter writer(dmi::DaemonMessage::LaunchApplication);
                                        writer.Write<u64>(title.app_id);
                                    }

                                    dmi::MenuResultReader reader;
                                    if(reader && R_SUCCEEDED(reader.GetResult())) {
                                        g_menu_app_instance->StopPlayBGM();
                                        g_menu_app_instance->CloseWithFadeOut();
                                        return;
                                    }
                                    else {
                                        auto rc = reader.GetResult();
                                        g_menu_app_instance->ShowNotification(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "app_launch_error") + ": " + util::FormatResult(rc));
                                    }
                                }
                            }
                        }
                        else if(down & KEY_X) {
                            if(g_menu_app_instance->IsSuspended()) {
                                if(type == cfg::TitleType::Homebrew) {
                                    if(g_menu_app_instance->EqualsSuspendedHomebrewPath(title.nro_target.nro_path)) {
                                        this->HandleCloseSuspended();
                                    }
                                }
                                else {
                                    if(title.app_id == g_menu_app_instance->GetSuspendedApplicationId()) {
                                        this->HandleCloseSuspended();
                                    }
                                }
                            }
                        }
                        else if(down & KEY_Y) {
                            if(type == cfg::TitleType::Homebrew) {
                                auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "entry_options"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "entry_action"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "entry_move"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "entry_remove"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "cancel") }, true);
                                if(sopt == 0) {
                                    if(!this->select_on) {
                                        this->select_on = true;
                                    }
                                    this->itemsMenu->SetItemMultiselected(this->itemsMenu->GetSelectedItem(), true);
                                }
                                else if(sopt == 1) {
                                    auto sopt2 = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "entry_remove"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "entry_remove_conf"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "yes"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "no") }, true);
                                    if(sopt2 == 0) {
                                        cfg::RemoveRecord(title);
                                        folder.titles.erase(folder.titles.begin() + titleidx);
                                        g_menu_app_instance->ShowNotification(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "entry_remove_ok"));
                                        this->MoveFolder(this->curfolder, true);
                                    }
                                }
                            }
                            else {
                                this->select_on = true;
                                this->itemsMenu->SetItemMultiselected(this->itemsMenu->GetSelectedItem(), true);
                            }
                        }
                        else if(down & KEY_UP) {
                            if(type == cfg::TitleType::Installed) {
                                // TODO: strings
                                auto sopt = g_menu_app_instance->CreateShowDialog("Homebrew title take-over", "Would you like to select this title for homebrew launching?\nIf selected, homebrew will be launched as an application over this title.", { "Yes", "Cancel" }, true);
                                if(sopt == 0) {
                                    g_ul_config.homebrew_title_application_id = title.app_id;
                                    cfg::SaveConfig(g_ul_config);
                                    g_menu_app_instance->ShowNotification("Done");
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void MenuLayout::menu_OnSelected(u32 index) {
        this->itemAuthor->SetVisible(true);
        this->itemVersion->SetVisible(true);
        u32 realidx = index;
        if(this->homebrew_mode) {
            if(index == 0) {
                this->itemAuthor->SetVisible(false);
                this->itemVersion->SetVisible(false);
                this->bannerImage->SetImage(cfg::GetAssetByTheme(g_ul_theme, "ui/BannerHomebrew.png"));
                this->itemName->SetText(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "hbmenu_launch"));
            }
            else {
                realidx--;
                auto hb = g_homebrew_records[realidx];
                auto info = cfg::GetRecordInformation(hb);

                if(info.strings.name.empty()) {
                    this->itemName->SetText(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "unknown"));
                }
                else {
                    this->itemName->SetText(info.strings.name);
                }

                if(info.strings.author.empty()) {
                    this->itemAuthor->SetText(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "unknown"));
                }
                else {
                    this->itemAuthor->SetText(info.strings.author);
                }

                if(info.strings.version.empty()) {
                    this->itemVersion->SetText("0");
                }
                else {
                    this->itemVersion->SetText(info.strings.version);
                }

                this->bannerImage->SetImage(cfg::GetAssetByTheme(g_ul_theme, "ui/BannerHomebrew.png"));
            }
        }
        else {
            auto &folder = cfg::FindFolderByName(g_entry_list, this->curfolder);
            s32 titleidx = realidx;
            if(this->curfolder.empty()) {
                if(realidx >= g_entry_list.folders.size()) {
                    titleidx -= g_entry_list.folders.size();
                }
                else {
                    auto foldr = g_entry_list.folders[realidx];
                    this->bannerImage->SetImage(cfg::GetAssetByTheme(g_ul_theme, "ui/BannerFolder.png"));
                    auto sz = foldr.titles.size();
                    this->itemAuthor->SetText(std::to_string(sz) + " " + ((sz == 1) ? cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "folder_entry_single") : cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "folder_entry_mult")));
                    this->itemVersion->SetVisible(false);
                    this->itemName->SetText(foldr.name);
                    titleidx = -1;
                }
            }
            if(titleidx >= 0) {
                auto title = folder.titles[titleidx];
                auto info = cfg::GetRecordInformation(title);

                if(info.strings.name.empty()) {
                    this->itemName->SetText(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "unknown"));
                }
                else {
                    this->itemName->SetText(info.strings.name);
                }

                if(info.strings.author.empty()) {
                    this->itemAuthor->SetText(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "unknown"));
                }
                else {
                    this->itemAuthor->SetText(info.strings.author);
                }

                if(info.strings.version.empty()) {
                    this->itemVersion->SetText("0");
                }
                else {
                    this->itemVersion->SetText(info.strings.version);
                }

                if(static_cast<cfg::TitleType>(title.title_type) == cfg::TitleType::Homebrew) {
                    this->bannerImage->SetImage(cfg::GetAssetByTheme(g_ul_theme, "ui/BannerHomebrew.png"));
                }
                else {
                    this->bannerImage->SetImage(cfg::GetAssetByTheme(g_ul_theme, "ui/BannerInstalled.png"));
                }
            }
            if(!this->curfolder.empty()) {
                // This way we know we're inside a folder
                this->bannerImage->SetImage(cfg::GetAssetByTheme(g_ul_theme, "ui/BannerFolder.png"));
            }
        }
    }

    void MenuLayout::MoveFolder(const std::string &name, bool fade) {
        if(fade) {
            g_menu_app_instance->FadeOut();
        }

        if(this->homebrew_mode) {
            if(g_homebrew_records.empty()) {
                g_homebrew_records = cfg::QueryAllHomebrew();
            }
        }

        auto itm_list = g_homebrew_records;
        if(!this->homebrew_mode) {
            auto &folder = cfg::FindFolderByName(g_entry_list, name);
            itm_list = folder.titles;
        }

        this->itemsMenu->ClearItems();
        if(this->homebrew_mode) {
            this->itemsMenu->AddItem(cfg::GetAssetByTheme(g_ul_theme, "ui/Hbmenu.png"));
        }
        else {
            if(name.empty()) {
                // Remove empty folders
                STL_REMOVE_IF(g_entry_list.folders, fldr, (fldr.titles.empty()))
                for(auto folder: g_entry_list.folders) {
                    this->itemsMenu->AddItem(cfg::GetAssetByTheme(g_ul_theme, "ui/Folder.png"), folder.name);
                }
            }
        }

        u32 tmpidx = 0;
        for(auto &itm: itm_list) {
            bool set_susp = false;
            if(static_cast<cfg::TitleType>(itm.title_type) == cfg::TitleType::Installed) {
                if(g_menu_app_instance->IsTitleSuspended()) {
                    if(g_menu_app_instance->GetSuspendedApplicationId() == itm.app_id) {
                        set_susp = true;
                    }
                }
            }
            else {
                if(g_menu_app_instance->IsHomebrewSuspended()) {
                    if(g_menu_app_instance->EqualsSuspendedHomebrewPath(itm.nro_target.nro_path)) {
                        set_susp = true;
                    }
                }
            }
            this->itemsMenu->AddItem(cfg::GetRecordIconPath(itm));
            if(set_susp) {
                u32 suspidx = tmpidx;
                // Skip initial item if g_homebrew_records mode
                if(this->homebrew_mode) {
                    suspidx++;
                }
                // Ignore front folders from main menu
                else if(name.empty()) {
                    suspidx += g_entry_list.folders.size();
                }
                this->itemsMenu->SetSuspendedItem(suspidx);
            }
            tmpidx++;
        }

        this->itemsMenu->UpdateBorderIcons();
        if(!this->homebrew_mode) {
            this->curfolder = name;
        }

        if(fade) {
            g_menu_app_instance->FadeIn();
        }
    }

    void MenuLayout::SetUser(AccountUid user) {
        auto path = os::GetIconCacheImagePath(user);
        this->users->SetImage(path);
        this->users->SetWidth(50);
        this->users->SetHeight(50);
    }

    void MenuLayout::menuToggle_Click() {
        pu::audio::Play(this->sfxMenuToggle);
        this->homebrew_mode = !this->homebrew_mode;
        if(this->select_on) {
            g_menu_app_instance->ShowNotification(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "menu_multiselect_cancel"));
            this->StopMultiselect();
        }
        this->MoveFolder("", true);
    }

    void MenuLayout::HandleCloseSuspended() {
        auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "suspended_app"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "suspended_close"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "yes"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "no") }, true);
        if(sopt == 0) {
            this->DoTerminateApplication();
        }
    }

    void MenuLayout::HandleHomebrewLaunch(cfg::TitleRecord &rec) {
        u32 launchmode = 0;
        if(g_ul_config.system_title_override_enabled) {
            auto sopt = g_menu_app_instance->CreateShowDialog(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "hb_launch"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "hb_launch_conf"), { cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "hb_applet"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "hb_app"), cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "cancel") }, true);
            if(sopt == 0) {
                launchmode = 1;
            }
            else if(sopt == 1) {
                launchmode = 2;
            }
        }
        else {
            launchmode = 1;
        }
        if(launchmode == 1) {
            pu::audio::Play(this->sfxTitleLaunch);
            hb::HbTargetParams ipt = {};
            strcpy(ipt.nro_path, rec.nro_target.nro_path);
            strcpy(ipt.nro_argv, rec.nro_target.nro_argv);
            if(strlen(rec.nro_target.nro_argv)) {
                sprintf(ipt.nro_argv, "%s %s", rec.nro_target.nro_path, rec.nro_target.nro_argv);
            }

            dmi::MenuMessageWriter writer(dmi::DaemonMessage::LaunchHomebrewLibraryApplet);
            writer.Write<hb::HbTargetParams>(ipt);

            g_menu_app_instance->StopPlayBGM();
            g_menu_app_instance->CloseWithFadeOut();
            return;
        }
        else if(launchmode == 2) {
            if(g_ul_config.homebrew_title_application_id != 0)
            {
                bool launch = true;
                if(g_menu_app_instance->IsSuspended()) {
                    launch = false;
                    this->HandleCloseSuspended();
                    if(!g_menu_app_instance->IsSuspended()) {
                        launch = true;
                    }
                }
                if(launch) {
                    pu::audio::Play(this->sfxTitleLaunch);
                    hb::HbTargetParams ipt = {};
                    strcpy(ipt.nro_path, rec.nro_target.nro_path);
                    strcpy(ipt.nro_argv, rec.nro_target.nro_argv);
                    if(strlen(rec.nro_target.nro_argv)) {
                        sprintf(ipt.nro_argv, "%s %s", rec.nro_target.nro_path, rec.nro_target.nro_argv);
                    }

                    dmi::MenuMessageWriter writer(dmi::DaemonMessage::LaunchHomebrewApplication);
                    {
                        writer.Write<u64>(g_ul_config.homebrew_title_application_id);
                        writer.Write<hb::HbTargetParams>(ipt);
                    }

                    dmi::MenuResultReader reader;
                    if(reader && R_SUCCEEDED(reader.GetResult())) {
                        g_menu_app_instance->StopPlayBGM();
                        g_menu_app_instance->CloseWithFadeOut();
                        return;
                    }
                    else {
                        auto rc = reader.GetResult();
                        g_menu_app_instance->ShowNotification(cfg::GetLanguageString(g_ul_config.main_lang, g_ul_config.default_lang, "app_launch_error") + ": " + util::FormatResult(rc));
                    }
                    reader.FinishRead();
                }
            }
            else {
                g_menu_app_instance->CreateShowDialog("Launch", "There is no title specified for homebrew to take over it.\nSelect one by pressing up over it.", { "Ok" }, true);
            }
        }
    }

    void MenuLayout::HandleMultiselectMoveToFolder(const std::string &folder) {
        if(this->select_on) {
            u32 rmvd = 0;
            auto basesz = g_entry_list.root.titles.size();
            auto basefsz = g_entry_list.folders.size();
            for(u32 i = 0; i < basesz; i++) {
                auto &title = g_entry_list.root.titles[i - rmvd];
                if(this->itemsMenu->IsItemMultiselected(basefsz + i)) {
                    if(cfg::MoveRecordTo(g_entry_list, title, folder)) {
                        rmvd++;
                    }
                }
            }
            this->StopMultiselect();
            this->MoveFolder(this->curfolder, true);
        }
    }

    void MenuLayout::StopMultiselect() {
        this->select_on = false;
        this->select_dir = false;
        this->itemsMenu->ResetMultiselections();
    }

    void MenuLayout::DoTerminateApplication() {
        this->itemsMenu->UnsetSuspendedItem();
        g_menu_app_instance->NotifyEndSuspended();
        this->bgSuspendedRaw->SetAlphaFactor(0);

        dmi::MenuMessageWriter writer(dmi::DaemonMessage::TerminateApplication);
    }

}