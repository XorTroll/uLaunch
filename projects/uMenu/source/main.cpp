#include <ul/fs/fs_Stdio.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/util/util_Json.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/util/util_Size.hpp>
#include <ul/net/net_Service.hpp>
#include <ul/menu/smi/smi_MenuMessageHandler.hpp>
#include <ul/menu/am/am_LibraryAppletUtils.hpp>
#include <ul/menu/am/am_LibnxLibappletWrap.hpp>

using namespace ul::util::size;

SetSysFirmwareVersion g_FwVersion;

extern "C" {

    AppletType __nx_applet_type = AppletType_LibraryApplet; // Explicitly declare we're a library applet (need to do so for non-hbloader homebrew)
    TimeServiceType __nx_time_service_type = TimeServiceType_User;
    u32 __nx_fs_num_sessions = 1;
    size_t __nx_heap_size = 296_MB;

    void __libnx_init_time();
    void __libnx_init_cwd();

    void __nx_win_init();
    void __nx_win_exit();

    void __appInit() {
        UL_RC_ASSERT(smInitialize());

        UL_RC_ASSERT(fsInitialize());
        UL_RC_ASSERT(fsdevMountSdmc());

        UL_RC_ASSERT(setsysInitialize());
        UL_RC_ASSERT(setsysGetFirmwareVersion(&g_FwVersion));
        hosversionSet(MAKEHOSVERSION(g_FwVersion.major, g_FwVersion.minor, g_FwVersion.micro) | BIT(31));
        setsysExit();

        UL_RC_ASSERT(appletInitialize());
        UL_RC_ASSERT(hidInitialize());
        UL_RC_ASSERT(timeInitialize());
        __libnx_init_time();

        UL_RC_ASSERT(accountInitialize(AccountServiceType_System));
        UL_RC_ASSERT(nsInitialize());
        UL_RC_ASSERT(ul::net::Initialize());
        UL_RC_ASSERT(psmInitialize());
        UL_RC_ASSERT(setsysInitialize());
        UL_RC_ASSERT(setInitialize());

        __nx_win_init();
    }

    void __appExit() {
        __nx_win_exit();

        setExit();
        setsysExit();
        psmExit();
        ul::net::Finalize();
        nsExit();
        accountExit();

        timeExit();

        hidExit();

        appletExit();

        fsdevUnmountAll();
        fsExit();

        smExit();
    }

}

ul::menu::ui::MenuApplication::Ref g_MenuApplication;

ul::cfg::Config g_Config;
ul::cfg::Theme g_ActiveTheme;

ul::util::JSON g_DefaultLanguage;
ul::util::JSON g_MainLanguage;

namespace {

    ul::smi::MenuStartMode g_StartMode;
    ul::smi::SystemStatus g_SystemStatus;

    void MainLoop() {
        // After initializing RomFs, start initializing the rest of stuff here
        ul::menu::InitializeEntries();

        // Load menu config
        g_Config = ul::cfg::LoadConfig();

        // Load active theme if set
        std::string active_theme_name;
        UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::ActiveThemeName, active_theme_name));
        if(!active_theme_name.empty()) {
            const auto rc = ul::cfg::TryLoadTheme(active_theme_name, g_ActiveTheme);
            if(R_SUCCEEDED(rc)) {
                ul::cfg::EnsureCacheActiveTheme(g_Config);
            }
            else {
                g_ActiveTheme = {};
                UL_LOG_WARN("Unable to load active entry '%s', resetting to default: %s", active_theme_name.c_str(), ul::util::FormatResultDisplay(rc).c_str());
                UL_ASSERT_TRUE(g_Config.SetEntry(ul::cfg::ConfigEntryId::ActiveThemeName, g_ActiveTheme.name));
                ul::cfg::RemoveActiveThemeCache();
            }
        }
        else {
            UL_LOG_INFO("No active theme set...");
        }

        // Get system language and load translations (default one if not present)
        ul::cfg::LoadLanguageJsons(ul::MenuLanguagesPath, g_MainLanguage, g_DefaultLanguage);

        // Get the text sizes to initialize default fonts
        auto ui_json = ul::util::JSON::object();
        UL_RC_ASSERT(ul::util::LoadJSONFromFile(ui_json, ul::menu::ui::TryGetActiveThemeResource("ui/UI.json")));

        auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);

        const auto default_font_path = ul::menu::ui::TryGetActiveThemeResource("ui/Font.ttf");
        if(!default_font_path.empty()) {
            renderer_opts.AddDefaultFontPath(default_font_path);
        }
        else {
            renderer_opts.AddDefaultSharedFont(PlSharedFontType_Standard);
            renderer_opts.AddDefaultSharedFont(PlSharedFontType_ChineseSimplified);
            renderer_opts.AddDefaultSharedFont(PlSharedFontType_ExtChineseSimplified);
            renderer_opts.AddDefaultSharedFont(PlSharedFontType_ChineseTraditional);
            renderer_opts.AddDefaultSharedFont(PlSharedFontType_KO);
        }
        renderer_opts.AddDefaultSharedFont(PlSharedFontType_NintendoExt);

        renderer_opts.UseImage(pu::ui::render::IMGAllFlags);
        renderer_opts.UseAudio(pu::ui::render::MixerAllFlags);

        auto renderer = pu::ui::render::Renderer::New(renderer_opts);
        g_MenuApplication = ul::menu::ui::MenuApplication::New(renderer);

        g_MenuApplication->Initialize(g_StartMode, g_SystemStatus, ui_json);
        g_MenuApplication->Prepare();

        // With the handlers ready, initialize uSystem message handling
        UL_RC_ASSERT(ul::menu::smi::InitializeMenuMessageHandler());

        if(g_StartMode == ul::smi::MenuStartMode::MainMenuApplicationSuspended) {
            g_MenuApplication->Show();
        }
        else {
            g_MenuApplication->ShowWithFadeIn();
        }

        g_MenuApplication = {};
    }

}

// uMenu procedure: read sent storages, initialize RomFs (externally), load config and other stuff, finally create the renderer and start the UI

int main() {
    ul::InitializeLogging("uMenu");
    UL_LOG_INFO("Alive!");

    UL_RC_ASSERT(ul::menu::am::ReadStartMode(g_StartMode));
    UL_ASSERT_TRUE(g_StartMode != ul::smi::MenuStartMode::Invalid);

    UL_LOG_INFO("Start mode: %d", (u32)g_StartMode);

    // Information sent as an extra storage to uMenu
    UL_RC_ASSERT(ul::menu::am::ReadFromInputStorage(&g_SystemStatus, sizeof(g_SystemStatus)));
    
    // Check if our RomFs data exists...
    if(!ul::fs::ExistsFile(ul::MenuRomfsFile)) {
        UL_RC_ASSERT(ul::ResultRomfsNotFound);
    }

    // Try to mount it
    UL_RC_ASSERT(romfsMountFromFsdev(ul::MenuRomfsFile, 0, "romfs"));

    // Register handlers for HOME button press detection
    ul::menu::am::RegisterLibnxLibappletHomeButtonDetection();
    ul::menu::ui::RegisterMenuOnMessageDetect();
    ul::menu::ui::QuickMenu::RegisterHomeButtonDetection();

    MainLoop();

    ul::menu::smi::FinalizeMenuMessageHandler();

    // Exit RomFs manually, since we also initialized it manually
    romfsExit();

    UL_LOG_INFO("Goodbye!");
    return 0;
}
