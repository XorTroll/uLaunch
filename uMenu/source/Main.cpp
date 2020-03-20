#include <pu/Plutonium>
#include <ctime>
#include <chrono>
#include <sstream>
#include <thread>
#include <db/db_Save.hpp>
#include <fs/fs_Stdio.hpp>
#include <cfg/cfg_Config.hpp>
#include <net/net_Service.hpp>
#include <util/util_Misc.hpp>
#include <ui/ui_MenuApplication.hpp>
#include <os/os_HomeMenu.hpp>
#include <util/util_Convert.hpp>
#include <am/am_LibraryApplet.hpp>
#include <am_DaemonMessages.hpp>
#include <am_LibAppletWrap.hpp>

extern "C"
{
    u32 __nx_applet_type = AppletType_LibraryApplet; // Explicitly declare we're a library applet (need to do so for non-hbloader homebrew)
    TimeServiceType __nx_time_service_type = TimeServiceType_System;
    size_t __nx_heap_size = 0xD000000; // 208MB heap
}

#define MENU_ROMFS_BIN UL_BASE_SD_DIR "/bin/uMenu/romfs.bin"

// Some global vars

ui::MenuApplication::Ref g_menu_app_instance;
cfg::TitleList g_entry_list;
std::vector<cfg::TitleRecord> g_homebrew_records;
cfg::Config g_ul_config;
cfg::Theme g_ul_theme;
u8 *g_app_capture_buffer;

namespace qmenu
{
    void Initialize()
    {
        UL_ASSERT(accountInitialize(AccountServiceType_System))
        UL_ASSERT(nsInitialize())
        UL_ASSERT(net::Initialize())
        UL_ASSERT(psmInitialize())
        UL_ASSERT(setsysInitialize())
        UL_ASSERT(setInitialize())

        // Register handlers for HOME button press detection
        am::RegisterLibAppletHomeButtonDetection();
        ui::RegisterHomeButtonDetection();
        ui::QuickMenu::RegisterHomeButtonDetection();

        // Initialize Daemon message handling
        UL_ASSERT(am::InitializeDaemonMessageHandler());

        // Load menu g_ul_config and theme
        g_ul_config = cfg::EnsureConfig();
        g_ul_theme = cfg::LoadTheme(g_ul_config.theme_name);
    }

    void Exit()
    {
        am::ExitDaemonMessageHandler();

        setExit();
        setsysExit();
        psmExit();
        net::Finalize();
        nsExit();
        accountExit();
    }
}

int main()
{
    auto [rc, smode] = am::Menu_ProcessInput();
    if(R_SUCCEEDED(rc))
    {
        am::DaemonStatus status = {};
        
        // Information block sent as an extra storage to Menu.
        am::Menu_DaemonReadImpl(&status, sizeof(status), false);

        // First read sent storages, then init the renderer (UI, which also inits RomFs), then init everything else

        if(smode != am::MenuStartMode::Invalid)
        {
            // Check if our RomFs file exists...
            if(!fs::ExistsFile(MENU_ROMFS_BIN)) UL_ASSERT(RES_VALUE(Menu, RomfsBinNotFound))

            // Try to mount RomFs from our binary
            UL_ASSERT(romfsMountFromFsdev(MENU_ROMFS_BIN, 0, "romfs"))

            // After initializing RomFs, start initializing the rest of stuff here
            g_app_capture_buffer = new u8[RawRGBAScreenBufferSize]();
            qmenu::Initialize();
            
            g_entry_list = cfg::LoadTitleList();

            // Get system language and load translations (default one if not present)
            u64 lcode = 0;
            setGetLanguageCode(&lcode);
            std::string syslang = (char*)&lcode;
            auto lpath = cfg::GetLanguageJSONPath(syslang);
            auto [rc1, defjson] = util::LoadJSONFromFile(CFG_LANG_DEFAULT);
            UL_ASSERT(rc1);
            g_ul_config.default_lang = defjson;
            g_ul_config.main_lang = defjson;
            if(fs::ExistsFile(lpath))
            {
                auto [rc2, ljson] = util::LoadJSONFromFile(lpath);
                if(R_SUCCEEDED(rc2)) g_ul_config.main_lang = ljson;
            }

            // Get the text sizes to initialize default fonts
            auto [rc2, uijson] = util::LoadJSONFromFile(cfg::GetAssetByTheme(g_ul_theme, "ui/UI.json"));
            UL_ASSERT(rc2);
            auto menu_folder_txt_sz = uijson.value<u32>("menu_folder_text_size", 25);

            auto renderer = pu::ui::render::Renderer::New(pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags).WithIMG(pu::ui::render::IMGAllFlags).WithMixer(pu::ui::render::MixerAllFlags).WithTTF().WithDefaultFontSize(menu_folder_txt_sz));
            g_menu_app_instance = ui::MenuApplication::New(renderer);

            g_menu_app_instance->SetInformation(smode, status, uijson);
            g_menu_app_instance->Prepare();
            
            if(smode == am::MenuStartMode::MenuApplicationSuspended) g_menu_app_instance->Show();
            else g_menu_app_instance->ShowWithFadeIn();

            // Exit RomFs manually, as we initialized it manually too
            romfsExit();

            delete[] g_app_capture_buffer;
            qmenu::Exit();
        }
    }

    return 0;
}