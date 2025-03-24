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

ul::menu::ui::GlobalSettings g_GlobalSettings;

namespace {

    constexpr u32 ExosphereApiVersionConfigItem = 65000;
    constexpr u32 ExosphereEmummcType = 65007;

}

extern "C" {

    AppletType __nx_applet_type = AppletType_LibraryApplet; // Explicitly declare we're a library applet (need to do so for non-hbloader homebrew)
    TimeServiceType __nx_time_service_type = TimeServiceType_Menu;
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

        UL_RC_ASSERT(splInitialize());
        // Since we rely on ams for uLaunch to work, it *must* be present
        u64 raw_ams_ver;
        UL_RC_ASSERT(splGetConfig(static_cast<SplConfigItem>(ExosphereApiVersionConfigItem), &raw_ams_ver));
        g_GlobalSettings.ams_version = {
            .major = static_cast<u8>((raw_ams_ver >> 56) & 0xFF),
            .minor = static_cast<u8>((raw_ams_ver >> 48) & 0xFF),
            .micro = static_cast<u8>((raw_ams_ver >> 40) & 0xFF)
        };
        u64 emummc_type;
        UL_RC_ASSERT(splGetConfig(static_cast<SplConfigItem>(ExosphereEmummcType), &emummc_type));
        g_GlobalSettings.ams_is_emummc = emummc_type != 0;
        splExit();

        UL_RC_ASSERT(setsysInitialize());
        UL_RC_ASSERT(setInitialize());

        UL_RC_ASSERT(setsysGetFirmwareVersion(&g_GlobalSettings.fw_version));
        hosversionSet(MAKEHOSVERSION(g_GlobalSettings.fw_version.major, g_GlobalSettings.fw_version.minor, g_GlobalSettings.fw_version.micro) | BIT(31));

        UL_RC_ASSERT(setsysGetSerialNumber(&g_GlobalSettings.serial_no));
        UL_RC_ASSERT(setsysGetSleepSettings(&g_GlobalSettings.sleep_settings));
        UL_RC_ASSERT(setGetRegionCode(&g_GlobalSettings.region));
        UL_RC_ASSERT(setsysGetPrimaryAlbumStorage(&g_GlobalSettings.album_storage));
        UL_RC_ASSERT(setsysGetNfcEnableFlag(&g_GlobalSettings.nfc_enabled));
        UL_RC_ASSERT(setsysGetUsb30EnableFlag(&g_GlobalSettings.usb30_enabled));
        UL_RC_ASSERT(setsysGetBluetoothEnableFlag(&g_GlobalSettings.bluetooth_enabled));
        UL_RC_ASSERT(setsysGetWirelessLanEnableFlag(&g_GlobalSettings.wireless_lan_enabled));
        UL_RC_ASSERT(setsysGetAutoUpdateEnableFlag(&g_GlobalSettings.auto_update_enabled));
        UL_RC_ASSERT(setsysGetAutomaticApplicationDownloadFlag(&g_GlobalSettings.auto_app_download_enabled));
        UL_RC_ASSERT(setsysGetConsoleInformationUploadFlag(&g_GlobalSettings.console_info_upload_enabled));
        u64 lang_code;
        UL_RC_ASSERT(setGetSystemLanguage(&lang_code));
        UL_RC_ASSERT(setMakeLanguage(lang_code, &g_GlobalSettings.language));
        s32 tmp;
        UL_RC_ASSERT(setGetAvailableLanguageCodes(&tmp, g_GlobalSettings.available_language_codes, ul::os::LanguageNameCount));
        UL_RC_ASSERT(setsysGetDeviceNickname(&g_GlobalSettings.nickname));
        UL_RC_ASSERT(setsysGetBatteryLot(&g_GlobalSettings.battery_lot));

        UL_RC_ASSERT(appletInitialize());
        UL_RC_ASSERT(hidInitialize());

        UL_RC_ASSERT(timeInitialize());
        __libnx_init_time();
        UL_RC_ASSERT(timeGetDeviceLocationName(&g_GlobalSettings.timezone));

        UL_RC_ASSERT(accountInitialize(AccountServiceType_System));
        UL_RC_ASSERT(nsInitialize());
        UL_RC_ASSERT(nssuInitialize());
        UL_RC_ASSERT(avmInitialize());
        UL_RC_ASSERT(ul::net::Initialize());
        UL_RC_ASSERT(psmInitialize());

        __nx_win_init();
    }

    void __appExit() {
        ul::menu::smi::FinalizeMenuMessageHandler();

        // Exit RomFs manually, since we also initialized it manually
        romfsExit();

        UL_LOG_INFO("Goodbye!");

        __nx_win_exit();

        setExit();
        setsysExit();
        psmExit();
        ul::net::Finalize();
        avmExit();
        nssuExit();
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

namespace {

    ul::smi::MenuStartMode g_StartMode;

    void MainLoop() {
        // After initializing RomFs, start initializing the rest of stuff here
        ul::menu::InitializeEntries();

        // Load menu config
        g_GlobalSettings.config = ul::cfg::LoadConfig();

        // Cache active theme if needed
        if(g_GlobalSettings.system_status.reload_theme_cache) {
            ul::cfg::CacheActiveTheme(g_GlobalSettings.config);
        }

        UL_ASSERT_TRUE(g_GlobalSettings.config.GetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, g_GlobalSettings.cache_hb_takeover_app_id));

        // Load active theme if set
        std::string active_theme_name;
        UL_ASSERT_TRUE(g_GlobalSettings.config.GetEntry(ul::cfg::ConfigEntryId::ActiveThemeName, active_theme_name));
        if(!active_theme_name.empty()) {
            const auto rc = ul::cfg::TryLoadTheme(active_theme_name, g_GlobalSettings.active_theme);
            if(R_SUCCEEDED(rc)) {
                ul::cfg::EnsureCacheActiveTheme(g_GlobalSettings.config);
            }
            else {
                g_GlobalSettings.active_theme = {};
                UL_LOG_WARN("Unable to load active theme '%s': %s, resetting to default theme...", active_theme_name.c_str(), ul::util::FormatResultDisplay(rc).c_str());
                UL_ASSERT_TRUE(g_GlobalSettings.config.SetEntry(ul::cfg::ConfigEntryId::ActiveThemeName, g_GlobalSettings.active_theme.name));
                ul::cfg::RemoveActiveThemeCache();
            }
        }
        else {
            UL_LOG_INFO("No active theme set...");
        }

        // List added/removed/in verify applications
    
        UL_LOG_INFO("Added app count: %d", g_GlobalSettings.system_status.last_added_app_count);
        if(g_GlobalSettings.system_status.last_added_app_count > 0) {
            auto app_buf = new u64[g_GlobalSettings.system_status.last_added_app_count]();
            UL_RC_ASSERT(ul::menu::smi::ListAddedApplications(g_GlobalSettings.system_status.last_added_app_count, app_buf));
            for(u32 i = 0; i < g_GlobalSettings.system_status.last_added_app_count; i++) {
                UL_LOG_INFO("> Added app: 0x%016lX", app_buf[i]);
                g_GlobalSettings.added_app_ids.push_back(app_buf[i]);
            }
            delete[] app_buf;
        }

        UL_LOG_INFO("Deleted app count: %d", g_GlobalSettings.system_status.last_deleted_app_count);
        if(g_GlobalSettings.system_status.last_deleted_app_count > 0) {
            auto app_buf = new u64[g_GlobalSettings.system_status.last_deleted_app_count]();
            UL_RC_ASSERT(ul::menu::smi::ListDeletedApplications(g_GlobalSettings.system_status.last_deleted_app_count, app_buf));
            for(u32 i = 0; i < g_GlobalSettings.system_status.last_deleted_app_count; i++) {
                UL_LOG_INFO("> Deleted app: 0x%016lX", app_buf[i]);

                if(g_GlobalSettings.cache_hb_takeover_app_id == app_buf[i]) {
                    g_GlobalSettings.cache_hb_takeover_app_id = 0;
                    g_GlobalSettings.config.SetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, g_GlobalSettings.cache_hb_takeover_app_id);
                    g_GlobalSettings.SaveConfig();
                }

                g_GlobalSettings.deleted_app_ids.push_back(app_buf[i]);
            }
            delete[] app_buf;
        }

        UL_LOG_INFO("In verify app count: %d", g_GlobalSettings.system_status.in_verify_app_count);
        if(g_GlobalSettings.system_status.in_verify_app_count > 0) {
            auto app_buf = new u64[g_GlobalSettings.system_status.in_verify_app_count]();
            UL_RC_ASSERT(ul::menu::smi::ListInVerifyApplications(g_GlobalSettings.system_status.in_verify_app_count, app_buf));
            for(u32 i = 0; i < g_GlobalSettings.system_status.in_verify_app_count; i++) {
                UL_LOG_INFO("> App being verified: 0x%016lX", app_buf[i]);
                g_GlobalSettings.in_verify_app_ids.push_back(app_buf[i]);
            }
            delete[] app_buf;
        }

        // Get system language and load translations (default one if not present)
        ul::cfg::LoadLanguageJsons(ul::MenuLanguagesPath, g_GlobalSettings.main_lang, g_GlobalSettings.default_lang);

        auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
        renderer_opts.SetPlServiceType(PlServiceType_User);
        renderer_opts.SetInputPlayerCount(8);
        renderer_opts.AddInputNpadStyleTag(HidNpadStyleSet_NpadStandard);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_Handheld);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No1);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No2);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No3);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No4);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No5);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No6);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No7);
        renderer_opts.AddInputNpadIdType(HidNpadIdType_No8);

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
        renderer_opts.UseImage(pu::ui::render::ImgAllFlags);

        auto renderer = pu::ui::render::Renderer::New(renderer_opts);
        g_MenuApplication = ul::menu::ui::MenuApplication::New(renderer);

        UL_ASSERT_TRUE(pu::audio::Initialize(MIX_INIT_MP3));

        g_MenuApplication->Initialize(g_StartMode);
        UL_RC_ASSERT(g_MenuApplication->Load());

        // With the handlers ready, initialize uSystem message handling
        UL_RC_ASSERT(ul::menu::smi::InitializeMenuMessageHandler());

        if(g_StartMode == ul::smi::MenuStartMode::MainMenuApplicationSuspended) {
            g_MenuApplication->Show();
        }
        else {
            g_MenuApplication->ShowWithFadeIn();
        }
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
    UL_RC_ASSERT(ul::menu::am::ReadFromInputStorage(&g_GlobalSettings.system_status, sizeof(g_GlobalSettings.system_status)));
    g_GlobalSettings.initial_last_menu_index = g_GlobalSettings.system_status.last_menu_index;
    g_GlobalSettings.initial_last_menu_fs_path = g_GlobalSettings.system_status.last_menu_fs_path;

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

    return 0;
}
