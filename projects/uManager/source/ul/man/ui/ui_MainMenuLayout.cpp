#include <ul/man/ui/ui_MainApplication.hpp>
#include <ul/menu/menu_Cache.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/util/util_Json.hpp>
#include <ul/man/man_Manager.hpp>
#include <ul/man/man_Network.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/os/os_Applications.hpp>
#include <zzip/lib.h>

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

        constexpr size_t TemporaryFileExtractBufferSize = 0x10000;
        u8 g_TemporaryFileExtractBuffer[TemporaryFileExtractBufferSize];

    }

    MainMenuLayout::MainMenuLayout() : pu::ui::Layout() {
        this->info_text = pu::ui::elm::TextBlock::New(0, 25, "...");
        this->ResetInfoText();
        this->info_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::MediumLarge));
        this->info_text->SetHorizontalAlign(pu::ui::elm::HorizontalAlign::Center);
        this->info_text->SetColor(pu::ui::Color(0, 0, 0, 255));
        this->Add(this->info_text);

        this->options_menu = pu::ui::elm::Menu::New(0, 70, pu::ui::render::ScreenWidth, pu::ui::Color(80, 0, 120, 255), pu::ui::Color(127, 0, 190, 255), (pu::ui::render::ScreenHeight - 100) / MenuItemCount, MenuItemCount);
        
        this->activate_menu_item = pu::ui::elm::MenuItem::New(GetStatus());
        this->activate_menu_item->SetColor(pu::ui::Color(225, 225, 225, 255));
        this->activate_menu_item->AddOnKey(std::bind(&MainMenuLayout::activate_DefaultKey, this));
        this->options_menu->AddItem(this->activate_menu_item);

        this->reset_menu_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("reset_menu_item"));
        this->reset_menu_menu_item->SetColor(pu::ui::Color(225, 225, 225, 255));
        this->reset_menu_menu_item->AddOnKey(std::bind(&MainMenuLayout::resetMenu_DefaultKey, this));
        this->options_menu->AddItem(this->reset_menu_menu_item);

        this->reset_cache_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("reset_cache_item"));
        this->reset_cache_menu_item->SetColor(pu::ui::Color(225, 225, 225, 255));
        this->reset_cache_menu_item->AddOnKey(std::bind(&MainMenuLayout::resetCache_DefaultKey, this));
        this->options_menu->AddItem(this->reset_cache_menu_item);
        
        this->update_menu_item = pu::ui::elm::MenuItem::New(GetLanguageString("update_item"));
        this->update_menu_item->SetColor(pu::ui::Color(225, 225, 225, 255));
        this->update_menu_item->AddOnKey(std::bind(&MainMenuLayout::update_DefaultKey, this));
        this->options_menu->AddItem(this->update_menu_item);

        this->Add(this->options_menu);

        this->SetBackgroundColor(pu::ui::Color(192, 128, 217, 255));
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
            this->options_menu->ClearItems();
            this->options_menu->AddItem(this->activate_menu_item);
            this->options_menu->AddItem(this->update_menu_item);

            const auto option = g_MainApplication->CreateShowDialog(GetLanguageString("activate_changes_title"), GetLanguageString("activate_changes"), { GetLanguageString("reboot"), GetLanguageString("activate_continue") }, true);
            if(option == 0) {
                RebootSystem();
            }
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
            acc::CacheAccounts();

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
        const auto cur_ver = Version::FromString("0.3.1");
        if(last_ver.IsEqual(cur_ver)) {
            g_MainApplication->CreateShowDialog(GetLanguageString("update_title"), GetLanguageString("update_equal"), { GetLanguageString("ok") }, true);
        }
        else if(last_ver.IsLower(cur_ver)) {
            const auto option = g_MainApplication->CreateShowDialog(GetLanguageString("update_title"), GetLanguageString("update_version") + ": v" + last_ver.AsString() + "\n" + GetLanguageString("update_conf"), { GetLanguageString("yes"), GetLanguageString("cancel") }, true);
            if(option == 0) {
                this->options_menu->SetVisible(false);
                this->info_text->SetText(GetLanguageString("update_progress_download"));
                const auto download_url = "https://github.com/XorTroll/uLaunch/releases/download/" + last_id + "/uLaunch.zip";
                man::RetrieveToFile(download_url, TemporaryReleaseZipPath, [&](const double done, const double total) {
                    g_MainApplication->CallForRender();
                });
                
                this->info_text->SetText(GetLanguageString("update_progress_install"));
                auto zip_d = zzip_opendir(TemporaryReleaseZipPath);
                if(zip_d) {
                    while(true) {
                        auto zip_entry = zzip_readdir(zip_d);
                        if(zip_entry == nullptr) {
                            break;
                        }

                        std::string entry_name = zip_entry->d_name;
                        entry_name = entry_name.substr(__builtin_strlen("SdOut/"));
                        const auto is_dir = zip_entry->st_size == 0;
                        if(!entry_name.empty()) {
                            entry_name = "sdmc:/" + entry_name;
                            if(is_dir) {
                                mkdir(entry_name.c_str(), 777);
                            }
                            else {
                                remove(entry_name.c_str());

                                auto zip_f = zzip_file_open(zip_d, zip_entry->d_name, 0);
                                if(zip_f) {
                                    auto f = fopen(entry_name.c_str(), "wb");
                                    if(f) {
                                        auto rem_size = zip_entry->st_size;
                                        while(rem_size > 0) {
                                            const auto read_size = zzip_read(zip_f, g_TemporaryFileExtractBuffer, TemporaryFileExtractBufferSize);
                                            fwrite(g_TemporaryFileExtractBuffer, read_size, 1, f);
                                            rem_size -= read_size;
                                        }
                                        fclose(f);
                                    }
                                    zzip_close(zip_f);
                                }
                            }
                        }
                    }
                    zzip_closedir(zip_d);

                    g_MainApplication->CreateShowDialog(GetLanguageString("update_title"), GetLanguageString("update_success"), { GetLanguageString("reboot") }, true);
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