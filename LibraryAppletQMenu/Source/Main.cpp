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
cfg::ProcessedTheme theme;
u8 *app_buf;

namespace qmenu
{
    void Initialize()
    {
        accountInitialize();
        nsInitialize();
        net::Initialize();
        psmInitialize();
        setsysInitialize();
        setInitialize();

        am::QMenu_InitializeDaemonService();

        // Load menu config
        config = cfg::EnsureConfig();

        // Load theme
        auto th = cfg::LoadTheme(config.theme_name);
        theme = cfg::ProcessTheme(th);
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
        app_buf = new u8[RawRGBAScreenBufferSize]();
        qmenu::Initialize();
        auto [_rc, menulist] = cfg::LoadTitleList(true);
        list = menulist;

        if(smode != am::QMenuStartMode::Invalid)
        {
            auto renderer = pu::ui::render::Renderer::New(SDL_INIT_EVERYTHING, pu::ui::render::RendererInitOptions::RendererEverything, pu::ui::render::RendererHardwareFlags);
            qapp = ui::QMenuApplication::New(renderer);

            // Get system language and load translations (default one if not present)
            u64 lcode = 0;
            setGetLanguageCode(&lcode);
            std::string syslang = (char*)&lcode;
            auto lpath = cfg::GetLanguageJSONPath(syslang);
            auto [_rc, defjson1] = util::LoadJSONFromFile(CFG_LANG_DEFAULT);
            config.default_lang = defjson1;
            auto [_rc2, defjson2] = util::LoadJSONFromFile(CFG_LANG_DEFAULT);
            config.main_lang = defjson2;
            if(fs::ExistsFile(lpath))
            {
                auto [_rc3, ljson] = util::LoadJSONFromFile(lpath);
                config.main_lang = ljson;
            }

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