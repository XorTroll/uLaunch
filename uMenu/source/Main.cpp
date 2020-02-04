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

extern "C"
{
    u32 __nx_applet_type = AppletType_LibraryApplet; // Explicitly declare we're a library applet (need to do so for non-hbloader homebrew)
    TimeServiceType __nx_time_service_type = TimeServiceType_System;
    size_t __nx_heap_size = 0xD000000; // 208MB heap
}

#define MENU_ROMFS_BIN UL_BASE_SD_DIR "/bin/uMenu/romfs.bin"

// Some global vars

ui::MenuApplication::Ref qapp;
cfg::TitleList list;
std::vector<cfg::TitleRecord> homebrew;
cfg::Config config;
cfg::Theme theme;
u8 *app_buf;

namespace qmenu
{
    void Initialize()
    {
        UL_R_TRY(accountInitialize(AccountServiceType_System))
        UL_R_TRY(nsInitialize())
        UL_R_TRY(net::Initialize())
        UL_R_TRY(psmInitialize())
        UL_R_TRY(setsysInitialize())
        UL_R_TRY(setInitialize())

        UL_R_TRY(am::Menu_InitializeDaemonService())

        // Load menu config and theme
        config = cfg::EnsureConfig();
        theme = cfg::LoadTheme(config.theme_name);
    }

    void Exit()
    {
        am::Menu_FinalizeDaemonService();

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
            if(!fs::ExistsFile(MENU_ROMFS_BIN)) Panic("Unable to find RomFs binary: '" MENU_ROMFS_BIN "'");

            // Try to mount RomFs from our binary
            UL_R_TRY(romfsMountFromFsdev(MENU_ROMFS_BIN, 0, "romfs"))

            // After initializing RomFs, start initializing the rest of stuff here
            app_buf = new u8[RawRGBAScreenBufferSize]();
            qmenu::Initialize();
            
            list = cfg::LoadTitleList();

            // Get system language and load translations (default one if not present)
            u64 lcode = 0;
            setGetLanguageCode(&lcode);
            std::string syslang = (char*)&lcode;
            auto lpath = cfg::GetLanguageJSONPath(syslang);
            auto [rc1, defjson] = util::LoadJSONFromFile(CFG_LANG_DEFAULT);
            if(R_SUCCEEDED(rc1))
            {
                config.default_lang = defjson;
                config.main_lang = defjson;
                if(fs::ExistsFile(lpath))
                {
                    auto [rc2, ljson] = util::LoadJSONFromFile(lpath);
                    if(R_SUCCEEDED(rc2)) config.main_lang = ljson;
                }
            }

            auto renderoptions = pu::ui::render::RendererInitOptions::RendererEverything;
            renderoptions.InitRomFs = false; // We have loaded RomFs from an external file, so :P

            auto renderer = pu::ui::render::Renderer::New(SDL_INIT_EVERYTHING, renderoptions, pu::ui::render::RendererHardwareFlags);
            qapp = ui::MenuApplication::New(renderer);

            qapp->SetInformation(smode, status);
            qapp->Prepare();
            
            if(smode == am::MenuStartMode::MenuApplicationSuspended) qapp->Show();
            else qapp->ShowWithFadeIn();

            // Exit RomFs manually, Plutonium won't do it for us since we're initializing it manually
            romfsExit();

            delete[] app_buf;
            qmenu::Exit();
        }
    }

    return 0;
}