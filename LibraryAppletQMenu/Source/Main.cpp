#include <pu/Plutonium>
#include <ctime>
#include <chrono>
#include <sstream>
#include <thread>
#include <db/db_Save.hpp>
#include <fs/fs_Stdio.hpp>
#include <cfg/cfg_Config.hpp>
#include <ui/ui_QMenuApplication.hpp>

extern "C"
{
    u32 __nx_applet_type = AppletType_LibraryApplet; // Explicitly declare we're a library applet (need to do so for non-hbloader homebrew)
    size_t __nx_heap_size = 0x10000000; // 256MB heap - now we can use as much as we want from the applet pool ;)
}

// Some global vars

ui::QMenuApplication::Ref qapp;
cfg::TitleList list;
std::vector<cfg::TitleRecord> homebrew;
cfg::Config config;
cfg::ProcessedTheme theme;
u8 *app_buf;

namespace qmenu
{
    void Initialize()
    {
        accountInitialize();
        nsInitialize();
        nifmInitialize();
        psmInitialize();
        setsysInitialize();

        db::Mount();
        fs::CreateDirectory(Q_BASE_DB_DIR);
        fs::CreateDirectory(Q_BASE_SD_DIR);
        fs::CreateDirectory(Q_ENTRIES_PATH);
        fs::CreateDirectory(Q_THEMES_PATH);
        fs::CreateDirectory(Q_BASE_DB_DIR "/user");
        fs::CreateDirectory(Q_BASE_SD_DIR "/title");
        fs::CreateDirectory(Q_BASE_SD_DIR "/user");
        fs::CreateDirectory(Q_BASE_SD_DIR "/nro");
        db::Commit();

        am::QMenu_InitializeDaemonService();

        // Cache all homebrew (is this too slow...?)
        homebrew = cfg::QueryAllHomebrew(true);

        // Load menu config
        config = cfg::EnsureConfig();

        // Load theme
        auto th = cfg::LoadTheme(config.theme_name);
        theme = cfg::ProcessTheme(th);
    }

    void Exit()
    {
        am::QMenu_FinalizeDaemonService();

        db::Unmount();

        setsysExit();
        psmExit();
        nifmExit();
        nsExit();
        accountExit();
    }
}

int main()
{
    auto [rc, smode] = am::QMenu_ProcessInput();
    if(R_SUCCEEDED(rc))
    {
        app_buf = new u8[RawRGBAScreenBufferSize]();
        qmenu::Initialize();
        auto [_rc, menulist] = cfg::LoadTitleList(true);
        list = menulist;

        if(smode != am::QMenuStartMode::Invalid)
        {
            auto renderer = pu::ui::render::Renderer::New(SDL_INIT_EVERYTHING, pu::ui::render::RendererInitOptions::RendererEverything, pu::ui::render::RendererHardwareFlags);
            qapp = ui::QMenuApplication::New(renderer);

            qapp->SetStartMode(smode);
            qapp->Prepare();
            
            if(smode == am::QMenuStartMode::MenuApplicationSuspended) qapp->Show();
            else qapp->ShowWithFadeIn();
        }

        delete[] app_buf;
        qmenu::Exit();
    }

    return 0;
}