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
    size_t __nx_heap_size = 176_MB;

}

#define UL_MENU_ROMFS_BIN UL_BASE_SD_DIR "/bin/uMenu/romfs.bin"

ui::MenuApplication::Ref g_MenuApplication;
ui::TransitionGuard g_TransitionGuard;
u8 *g_ScreenCaptureBuffer;

cfg::TitleList g_EntryList;
std::vector<cfg::TitleRecord> g_HomebrewRecordList;

cfg::Config g_Config;
cfg::Theme g_Theme;

JSON g_DefaultLanguage;
JSON g_MainLanguage;
char g_FwVersion[0x18] = {};

namespace {

    void Initialize() {
        UL_ASSERT(accountInitialize(AccountServiceType_System));
        UL_ASSERT(nsInitialize());
        UL_ASSERT(net::Initialize());
        UL_ASSERT(psmInitialize());
        UL_ASSERT(setsysInitialize());
        UL_ASSERT(setInitialize());

        // Initialize uDaemon message handling
        UL_ASSERT(am::InitializeDaemonMessageHandler());

        // Load menu config and theme
        g_Config = cfg::LoadConfig();
        std::string theme_name;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::ActiveThemeName, theme_name));
        g_Theme = cfg::LoadTheme(theme_name);
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

// uMenu procedure: read sent storages, initialize RomFs (externally), load config and other stuff, finally create the renderer and start the UI

int main() {
    auto start_mode = dmi::MenuStartMode::Invalid;
    UL_ASSERT(am::ReadStartMode(start_mode));
    UL_ASSERT_TRUE(start_mode != dmi::MenuStartMode::Invalid);

    // Information sent as an extra storage to uMenu
    dmi::DaemonStatus status = {};
    UL_ASSERT(am::ReadDataFromStorage(&status, sizeof(status)));

    memcpy(g_FwVersion, status.fw_version, sizeof(g_FwVersion));
    
    // Check if our RomFs data exists...
    if(!fs::ExistsFile(UL_MENU_ROMFS_BIN)) {
        UL_ASSERT(menu::ResultRomfsFileNotFound);
    }

    // Try to mount it
    UL_ASSERT(romfsMountFromFsdev(UL_MENU_ROMFS_BIN, 0, "romfs"));

    // After initializing RomFs, start initializing the rest of stuff here
    g_ScreenCaptureBuffer = new u8[RawRGBAScreenBufferSize]();
    Initialize();

    // Cache title and homebrew icons
    cfg::CacheEverything();

    g_EntryList = cfg::LoadTitleList();

    // Get system language and load translations (default one if not present)
    u64 lang_code = 0;
    UL_ASSERT(setGetLanguageCode(&lang_code));
    std::string sys_lang = reinterpret_cast<char*>(&lang_code);
    auto lang_path = cfg::GetLanguageJSONPath(sys_lang);
    UL_ASSERT(util::LoadJSONFromFile(g_DefaultLanguage, CFG_LANG_DEFAULT));
    g_MainLanguage = g_DefaultLanguage;
    if(fs::ExistsFile(lang_path)) {
        auto lang_json = JSON::object();
        UL_ASSERT(util::LoadJSONFromFile(lang_json, lang_path));
        g_MainLanguage = lang_json;
    }

    // Get the text sizes to initialize default fonts
    auto uijson = JSON::object();
    UL_ASSERT(util::LoadJSONFromFile(uijson, cfg::GetAssetByTheme(g_Theme, "ui/UI.json")));
    const auto menu_folder_txt_sz = uijson.value<u32>("menu_folder_text_size", 25);

    const auto default_font_path = cfg::GetAssetByTheme(g_Theme, "ui/Font.ttf");
    auto renderer = pu::ui::render::Renderer::New(pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags).WithIMG(pu::ui::render::IMGAllFlags).WithMixer(pu::ui::render::MixerAllFlags).WithTTF(default_font_path).WithDefaultFontSize(menu_folder_txt_sz));
    g_MenuApplication = ui::MenuApplication::New(renderer);

    g_MenuApplication->SetInformation(start_mode, status, uijson);
    g_MenuApplication->Prepare();

    // Register handlers for HOME button press detection
    am::RegisterLibAppletHomeButtonDetection();
    ui::MenuApplication::RegisterHomeButtonDetection();
    ui::QuickMenu::RegisterHomeButtonDetection();
    
    if(start_mode == dmi::MenuStartMode::MenuApplicationSuspended) {
        g_MenuApplication->Show();
    }
    else {
        g_MenuApplication->ShowWithFadeIn();
    }

    // Exit RomFs manually, since we also initialized it manually
    romfsExit();

    delete[] g_ScreenCaptureBuffer;
    Exit();
    return 0;
}