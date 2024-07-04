#include <ul/man/ui/ui_MainApplication.hpp>
#include <ul/menu/menu_Cache.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/util/util_Json.hpp>
#include <ul/man/man_Manager.hpp>
#include <ul/man/man_Network.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/os/os_Applications.hpp>
#include <ul/util/util_Zip.hpp>

extern ul::man::ui::MainApplication::Ref g_MainApplication;
extern ul::util::JSON g_DefaultLanguage;
extern ul::util::JSON g_MainLanguage;

namespace ul::man::ui {

    namespace {

        inline std::string GetLanguageString(const std::string &name) {
            return cfg::GetLanguageString(g_MainLanguage, g_DefaultLanguage, name);
        }

        inline std::string GetStatus() {
            std::string status = GetLanguageString("status") + ": ";
            if(IsBasePresent()) {
                if(IsSystemActive()) {
                    status += GetLanguageString("status_active");
                }
                else {
                    status += GetLanguageString("status_not_active");
                }
            }
            else {
                status += GetLanguageString("status_not_present");
            }

            return status;
        }

        inline void RebootSystem() {
            UL_RC_ASSERT(spsmInitialize());
            spsmShutdown(true);
        }

        inline void ShowUpdateError() {
            g_MainApplication->CreateShowDialog(GetLanguageString("update_title"), GetLanguageString("update_error"), { GetLanguageString("ok") }, true);
        }

    }

    void MainMenuLayout::ResetInfoText() {
        std::string info = "uManager v" UL_VERSION ", uLaunch's manager";
        if(g_MainApplication->IsAvailable()) {
            const auto ver = g_MainApplication->GetVersion();
            info += " | running uLaunch v" + std::to_string((u32)ver.major) + "." + std::to_string((u32)ver.minor) + "." + std::to_string((u32)ver.micro);
            if(!g_MainApplication->IsVersionMatch()) {
                info += " (unexpected version)";
            }
        }
        this->info_text->SetText(info);
    }

    MainMenuLayout::MainMenuLayout() : pu::ui::Layout() {
        this->info_text = pu::ui::elm::TextBlock::New(0, InfoTextY, "...");
        this->ResetInfoText();
        this->info_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge));
        this->info_text->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->info_text->SetColor(InfoTextColor);
        this->Add(this->info_text);

        this->update_download_bar = pu::ui::elm::ProgressBar::New(0, UpdateDownloadBarY, UpdateDownloadBarWidth, UpdateDownloadBarHeight, 1.0f);
        this->update_download_bar->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->update_download_bar->SetVisible(false);
        this->Add(this->update_download_bar);

        this->options_menu = pu::ui::elm::Menu::New(0, MenuY, pu::ui::render::ScreenWidth, MenuColor, MenuFocusColor, MenuItemSize, MenuItemCount);
        
        this->activate_menu_item = pu::ui::elm::MenuItem::New(GetStatus());
        this->activate_menu_item->SetColor(MenuItemColor);
        this->activate_menu_item->AddOnKey(std::bind(&MainMenuLayout::activate_DefaultKey, this));
        this->options_menu->AddItem(this->activate_menu_item);

        this->reset_menu_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("reset_menu_item"));
        this->reset_menu_menu_item->SetColor(MenuItemColor);
        this->reset_menu_menu_item->AddOnKey(std::bind(&MainMenuLayout::resetMenu_DefaultKey, this));
        this->options_menu->AddItem(this->reset_menu_menu_item);

        this->reset_cache_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("reset_cache_item"));
        this->reset_cache_menu_item->SetColor(MenuItemColor);
        this->reset_cache_menu_item->AddOnKey(std::bind(&MainMenuLayout::resetCache_DefaultKey, this));
        this->options_menu->AddItem(this->reset_cache_menu_item);
        
        this->update_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("update_item"));
        this->update_menu_item->SetColor(MenuItemColor);
        this->update_menu_item->AddOnKey(std::bind(&MainMenuLayout::update_DefaultKey, this));
        this->options_menu->AddItem(this->update_menu_item);

        this->Add(this->options_menu);

        this->SetBackgroundColor(BackgroundColor);
    }

    void MainMenuLayout::activate_DefaultKey() {
        if(IsBasePresent()) {
            if(IsSystemActive()) {
                DeactivateSystem();
            }
            else {
                ActivateSystem();
            }

            this->activate_menu_item->SetName(GetStatus());
            this->ReloadMenu();

            g_MainApplication->CreateShowDialog(GetLanguageString("activate_changes_title"), GetLanguageString("activate_changes"), { GetLanguageString("reboot") }, true);
            g_MainApplication->FadeOut();
            RebootSystem();
        }
        else {
            g_MainApplication->CreateShowDialog(GetLanguageString("activate_changes_title"), GetLanguageString("activate_not_present"), { GetLanguageString("ok") }, true);
        }
    }

    void MainMenuLayout::resetMenu_DefaultKey() {
        const auto option = g_MainApplication->CreateShowDialog(GetLanguageString("reset_menu_title"), GetLanguageString("reset_menu_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
        if(option == 0) {
            fs::DeleteDirectory(MenuPath);

            // When returning to uMenu it will automatically regenerate the menu entries

            g_MainApplication->ShowNotification(GetLanguageString("reset_menu_success"));
        }
    }

    void MainMenuLayout::resetCache_DefaultKey() {
        const auto option = g_MainApplication->CreateShowDialog(GetLanguageString("reset_cache_title"), GetLanguageString("reset_cache_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
        if(option == 0) {
            // Regenerate cache
            const auto cur_app_recs = os::ListApplicationRecords();
            menu::CacheApplications(cur_app_recs);
            menu::CacheHomebrew();

            UL_RC_ASSERT(accountInitialize(AccountServiceType_System));
            UL_RC_ASSERT(acc::CacheAccounts());
            accountExit();

            cfg::RemoveActiveThemeCache();

            g_MainApplication->ShowNotification(GetLanguageString("reset_cache_success"));
        }
    }

    void MainMenuLayout::update_DefaultKey() {
        const auto json_data = man::RetrieveContent("https://api.github.com/repos/XorTroll/uLaunch/releases", "application/json");
        if(json_data.empty()) {
            ShowUpdateError();
            return;
        }

        const auto json = ul::util::JSON::parse(json_data);
        if(json.size() <= 0) {
            ShowUpdateError();
            return;
        }

        const auto last_id = json[0].value("tag_name", "");
        if(last_id.empty()) {
            ShowUpdateError();
            return;
        }

        const auto last_ver = Version::FromString(last_id);
        const auto cur_ver = Version::FromString(UL_VERSION);
        if(last_ver.IsEqual(cur_ver)) {
            g_MainApplication->CreateShowDialog(GetLanguageString("update_title"), GetLanguageString("update_equal"), { GetLanguageString("ok") }, true);
        }
        else if(last_ver.IsLower(cur_ver)) {
            const auto option = g_MainApplication->CreateShowDialog(GetLanguageString("update_title"), GetLanguageString("update_version") + ": v" + last_ver.AsString() + "\n" + GetLanguageString("update_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
            if(option == 0) {
                this->options_menu->SetVisible(false);
                this->update_download_bar->SetVisible(true);
                this->info_text->SetText(GetLanguageString("update_progress_download"));

                const auto download_url = "https://github.com/XorTroll/uLaunch/releases/download/" + last_id + "/uLaunch.zip";
                man::RetrieveToFile(download_url, TemporaryReleaseZipPath, [&](const double done, const double total) {
                    this->update_download_bar->SetMaxProgress(total);
                    this->update_download_bar->SetProgress(done);
                    g_MainApplication->CallForRender();
                });
                this->update_download_bar->SetProgress(0);
                
                this->info_text->SetText(GetLanguageString("update_progress_install"));

                auto zip_file = zip_open(TemporaryReleaseZipPath, 0, 'r');
                if(zip_file) {
                    const auto file_count = zip_entries_total(zip_file);
                    this->update_download_bar->SetMaxProgress((double)file_count);
                    if(file_count > 0) {
                        for(u32 i = 0; i < file_count; i++) {
                            if(zip_entry_openbyindex(zip_file, i) == 0) {
                                std::string entry_name = zip_entry_name(zip_file);
                                entry_name = entry_name.substr(__builtin_strlen("SdOut/"));
                                const auto is_dir = zip_entry_isdir(zip_file);
                                if(!entry_name.empty()) {
                                    entry_name = "sdmc:/" + entry_name;
                                    if(is_dir) {
                                        mkdir(entry_name.c_str(), 777);
                                    }
                                    else {
                                        auto f = fopen(entry_name.c_str(), "wb");
                                        if(f) {
                                            void *read_buf;
                                            size_t read_buf_size;
                                            const auto read_size = zip_entry_read(zip_file, &read_buf, &read_buf_size);
                                            fwrite(read_buf, read_size, 1, f);
                                            free(read_buf);
                                            fclose(f);
                                        }
                                    }
                                }
                            }
                            zip_entry_close(zip_file);
                            this->update_download_bar->SetProgress((double)(i + 1));
                        }
                    }
                    zip_close(zip_file);

                    this->update_download_bar->SetVisible(false);

                    g_MainApplication->CreateShowDialog(GetLanguageString("update_title"), GetLanguageString("update_success"), { GetLanguageString("reboot") }, true);
                    g_MainApplication->FadeOut();
                    RebootSystem();
                }
                else {
                    ShowUpdateError();
                }

                this->ResetInfoText();
                this->options_menu->SetVisible(true);
                remove(TemporaryReleaseZipPath);
            }
        }
        else if(last_ver.IsHigher(cur_ver)) {
            g_MainApplication->CreateShowDialog(GetLanguageString("update_title"), GetLanguageString("update_higher"), { GetLanguageString("ok") }, true);
        }
    }

}
