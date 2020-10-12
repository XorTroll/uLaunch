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
#include <am/am_DaemonMessages.hpp>
#include <am/am_LibAppletWrap.hpp>
#include <am/am_LibraryAppletUtils.hpp>

extern "C" {

    u32 __nx_applet_type = AppletType_LibraryApplet; // Explicitly declare we're a library applet (need to do so for non-hbloader homebrew)
    TimeServiceType __nx_time_service_type = TimeServiceType_System;
    u32 __nx_fs_num_sessions = 1;
    size_t __nx_heap_size = 0xD000000; // 208MB heap

}

#define UL_MENU_ROMFS_BIN UL_BASE_SD_DIR "/bin/uMenu/romfs.bin"

ui::MenuApplication::Ref g_MenuApplication;
cfg::TitleList g_EntryList;
std::vector<cfg::TitleRecord> g_HomebrewRecordList;
cfg::Config g_Config;
cfg::Theme g_Theme;
u8 *g_ScreenCaptureBuffer;

namespace impl {

    void Initialize() {
        UL_ASSERT(accountInitialize(AccountServiceType_System));
        UL_ASSERT(nsInitialize());
        UL_ASSERT(net::Initialize());
        UL_ASSERT(psmInitialize());
        UL_ASSERT(setsysInitialize());
        UL_ASSERT(setInitialize());

        // Register handlers for HOME button press detection
        am::RegisterLibAppletHomeButtonDetection();
        ui::MenuApplication::RegisterHomeButtonDetection();
        ui::QuickMenu::RegisterHomeButtonDetection();

        // Initialize uDaemon message handling
        UL_ASSERT(am::InitializeDaemonMessageHandler());

        // Load menu config and theme
        g_Config = cfg::EnsureConfig();
        g_Theme = cfg::LoadTheme(g_Config.theme_name);
    }

    void Exit() {
        am::ExitDaemonMessageHandler();

        setExit();
        setsysExit();
        psmExit();
        net::Finalize();
        nsExit();
        accountExit();
    }
}

int main() {
    // First read sent storages, then init the renderer (UI, which also inits RomFs), then init everything else
    auto smode = dmi::MenuStartMode::Invalid;
    
    auto rc = am::ReadStartMode(smode);
    if(R_SUCCEEDED(rc)) {
        dmi::DaemonStatus status = {};
        // Information block sent as an extra storage to uMenu.
        UL_ASSERT(am::ReadDataFromStorage(&status, sizeof(status)));
        
        if(smode != dmi::MenuStartMode::Invalid) {
            // Check if our RomFs data exists...
            if(!fs::ExistsFile(UL_MENU_ROMFS_BIN)) {
                UL_ASSERT(RES_VALUE(Menu, RomfsBinNotFound));
            }

            // Try to mount it
            UL_ASSERT(romfsMountFromFsdev(UL_MENU_ROMFS_BIN, 0, "romfs"));

            // After initializing RomFs, start initializing the rest of stuff here
            g_ScreenCaptureBuffer = new u8[RawRGBAScreenBufferSize]();
            impl::Initialize();

            g_EntryList = cfg::LoadTitleList();

            // Get system language and load translations (default one if not present)
            u64 lcode = 0;
            setGetLanguageCode(&lcode);
            std::string syslang = reinterpret_cast<char*>(&lcode);
            auto lpath = cfg::GetLanguageJSONPath(syslang);
            UL_ASSERT(util::LoadJSONFromFile(g_Config.default_lang, CFG_LANG_DEFAULT));
            g_Config.main_lang = g_Config.default_lang;
            if(fs::ExistsFile(lpath)) {
                auto ljson = JSON::object();
                UL_ASSERT(util::LoadJSONFromFile(ljson, lpath));
                g_Config.main_lang = ljson;
            }

            // Get the text sizes to initialize default fonts
            auto uijson = JSON::object();
            UL_ASSERT(util::LoadJSONFromFile(uijson, cfg::GetAssetByTheme(g_Theme, "ui/UI.json")));
            auto menu_folder_txt_sz = uijson.value<u32>("menu_folder_text_size", 25);

            auto renderer = pu::ui::render::Renderer::New(pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags).WithIMG(pu::ui::render::IMGAllFlags).WithMixer(pu::ui::render::MixerAllFlags).WithTTF().WithDefaultFontSize(menu_folder_txt_sz));
            g_MenuApplication = ui::MenuApplication::New(renderer);

            g_MenuApplication->SetInformation(smode, status, uijson);
            g_MenuApplication->Prepare();
            
            if(smode == dmi::MenuStartMode::MenuApplicationSuspended) {
                g_MenuApplication->Show();
            }
            else {
                g_MenuApplication->ShowWithFadeIn();
            }

            // Exit RomFs manually, since we also initialized it manually
            romfsExit();

            delete[] g_ScreenCaptureBuffer;
            impl::Exit();
        }
    }

    return 0;
}