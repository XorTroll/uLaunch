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
#include <ui/ui_QMenuApplication.hpp>
#include <os/os_HomeMenu.hpp>
#include <util/util_Convert.hpp>

extern "C"
{
    u32 __nx_applet_type = AppletType_LibraryApplet; // Explicitly declare we're a library applet (need to do so for non-hbloader homebrew)
    size_t __nx_heap_size = 0xD000000; // 208MB heap
}

// Some global vars

ui::QMenuApplication::Ref qapp;
cfg::TitleList list;
std::vector<cfg::TitleRecord> homebrew;
cfg::Config config;
cfg::Theme theme;
u8 *app_buf;

namespace qmenu
{
    void Initialize()
    {
        Q_R_TRY(accountInitialize(AccountServiceType_System))
        Q_R_TRY(nsInitialize())
        Q_R_TRY(net::Initialize())
        Q_R_TRY(psmInitialize())
        Q_R_TRY(setsysInitialize())
        Q_R_TRY(setInitialize())

        Q_R_TRY(am::QMenu_InitializeDaemonService())

        // Load menu config and theme
        config = cfg::EnsureConfig();
        theme = cfg::LoadTheme(config.theme_name);
    }

    void Exit()
    {
        am::QMenu_FinalizeDaemonService();

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
    auto [rc, smode] = am::QMenu_ProcessInput();
    if(R_SUCCEEDED(rc))
    {
        am::QDaemonStatus status = {};
        
        // Information block sent as an extra storage to QMenu.
        am::QLibraryAppletReadStorage(&status, sizeof(status));

        // First read sent storages, then init the renderer (UI, which also inits RomFs), then init everything else

        if(smode != am::QMenuStartMode::Invalid)
        {
            // Check if our RomFs file exists...
            if(!fs::ExistsFile(Q_MENU_ROMFS)) Panic("Unable to find RomFs binary: '" Q_MENU_ROMFS "'");

            // Try to mount RomFs from our binary
            Q_R_TRY(romfsMountFromFsdev(Q_MENU_ROMFS, 0, "romfs"))

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
            qapp = ui::QMenuApplication::New(renderer);

            qapp->SetInformation(smode, status);
            qapp->Prepare();
            
            if(smode == am::QMenuStartMode::MenuApplicationSuspended) qapp->Show();
            else qapp->ShowWithFadeIn();

            // Exit RomFs manually, Plutonium won't do it for us since we're initializing it manually
            romfsExit();

            delete[] app_buf;
            qmenu::Exit();
        }
    }

    return 0;
}