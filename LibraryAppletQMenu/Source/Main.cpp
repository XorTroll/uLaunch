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

ui::QMenuApplication::Ref qapp;
cfg::TitleList list;

namespace qmenu
{
    void Initialize()
    {
        accountInitialize();
        nsInitialize();
        db::Mount();
        am::QMenu_InitializeDaemonService();
    }

    void Exit()
    {
        am::QMenu_FinalizeDaemonService();
        db::Unmount();
        nsExit();
        accountExit();
    }
}

u8 *app_buf;

extern "C"
{
    void userAppInit(void)
    {
        qmenu::Initialize();
    }

    void userAppExit(void)
    {
        delete[] app_buf;
        qmenu::Exit();
    }
}

int main()
{
    app_buf = new u8[1280 * 720 * 4]();
    fs::CreateDirectory(Q_BASE_DB_DIR);
    fs::CreateDirectory(Q_BASE_DB_DIR "/user");
    auto [_rc, menulist] = cfg::LoadTitleList(true);
    list = menulist;

    auto [rc, smode] = am::QMenu_ProcessInput();
    if(R_SUCCEEDED(rc))
    {
        if(smode != am::QMenuStartMode::Invalid)
        {
            auto renderer = pu::ui::render::Renderer::New(SDL_INIT_EVERYTHING, pu::ui::render::RendererInitOptions::RendererEverything, pu::ui::render::RendererHardwareFlags);
            qapp = ui::QMenuApplication::New(renderer);

            if(smode == am::QMenuStartMode::MenuApplicationSuspended)
            {
                /*
                auto app_id = reader.Read<u64>();
                qapp->SetSuspendedApplicationId(app_id);
                */

                FILE *f = fopen(Q_BASE_SD_DIR "/temp-suspended.rgba", "rb");
                if(f)
                {
                    fread(app_buf, 1, 1280 * 720 * 4, f);
                    fclose(f);
                }
            }

            qapp->SetStartMode(smode);
            qapp->Prepare();
            
            if(smode == am::QMenuStartMode::MenuApplicationSuspended) qapp->Show();
            else qapp->ShowWithFadeIn();
        }
    }

    return 0;
}