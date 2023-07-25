#include <ul/fs/fs_Stdio.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/util/util_Json.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/util/util_Size.hpp>
#include <ul/net/net_Service.hpp>
#include <ul/menu/smi/smi_MenuMessageHandler.hpp>
#include <ul/menu/am/am_LibraryAppletUtils.hpp>
#include <ul/menu/am/am_LibnxLibappletWrap.hpp>
#include <ul/menu/menu_Results.hpp>

using namespace ul::util::size;

extern "C" {

    u32 __nx_applet_type = AppletType_LibraryApplet; // Explicitly declare we're a library applet (need to do so for non-hbloader homebrew)
    TimeServiceType __nx_time_service_type = TimeServiceType_System;
    u32 __nx_fs_num_sessions = 1;
    size_t __nx_heap_size = 176_MB;

}

constexpr const char RomfsFile[] = "sdmc:/ulaunch/bin/uMenu/romfs.bin";

ul::menu::ui::MenuApplication::Ref g_MenuApplication;
ul::menu::ui::TransitionGuard g_TransitionGuard;

ul::cfg::Config g_Config;
ul::cfg::Theme g_Theme;

ul::util::JSON g_DefaultLanguage;
ul::util::JSON g_MainLanguage;

namespace {

    void Initialize() {
        UL_RC_ASSERT(accountInitialize(AccountServiceType_System));
        UL_RC_ASSERT(nsInitialize());
        UL_RC_ASSERT(ul::net::Initialize());
        UL_RC_ASSERT(psmInitialize());
        UL_RC_ASSERT(setsysInitialize());
        UL_RC_ASSERT(setInitialize());

        ul::menu::InitializeEntries();

        // Load menu config and theme
        g_Config = ul::cfg::LoadConfig();
        std::string theme_name;
        UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::ActiveThemeName, theme_name));
        g_Theme = ul::cfg::LoadTheme(theme_name);
    }

    void Exit() {
        ul::menu::smi::FinalizeMenuMessageHandler();

        setExit();
        setsysExit();
        psmExit();
        ul::net::Finalize();
        nsExit();
        accountExit();
    }
}

// uMenu procedure: read sent storages, initialize RomFs (externally), load config and other stuff, finally create the renderer and start the UI

int main() {
    ul::InitializeLogging("uMenu");
    UL_LOG_INFO("Alive!");

    auto start_mode = ul::smi::MenuStartMode::Invalid;
    UL_RC_ASSERT(ul::menu::am::ReadStartMode(start_mode));
    UL_ASSERT_TRUE(start_mode != ul::smi::MenuStartMode::Invalid);

    // Information sent as an extra storage to uMenu
    ul::smi::SystemStatus status = {};
    UL_RC_ASSERT(ul::menu::am::ReadFromInputStorage(&status, sizeof(status)));
    
    // Check if our RomFs data exists...
    if(!ul::fs::ExistsFile(RomfsFile)) {
        UL_RC_ASSERT(ul::menu::ResultRomfsNotFound);
    }

    // Try to mount it
    UL_RC_ASSERT(romfsMountFromFsdev(RomfsFile, 0, "romfs"));

    // After initializing RomFs, start initializing the rest of stuff here
    Initialize();

    // Get system language and load translations (default one if not present)
    u64 lang_code = 0;
    UL_RC_ASSERT(setGetLanguageCode(&lang_code));
    const auto lang_path = ul::cfg::GetLanguageJSONPath(reinterpret_cast<char*>(&lang_code));
    UL_RC_ASSERT(ul::util::LoadJSONFromFile(g_DefaultLanguage, ul::DefaultLanguagePath));
    g_MainLanguage = g_DefaultLanguage;
    if(ul::fs::ExistsFile(lang_path)) {
        auto lang_json = ul::util::JSON::object();
        UL_RC_ASSERT(ul::util::LoadJSONFromFile(lang_json, lang_path));
        g_MainLanguage = lang_json;
    }

    // Get the text sizes to initialize default fonts
    auto ui_json = ul::util::JSON::object();
    UL_RC_ASSERT(ul::util::LoadJSONFromFile(ui_json, ul::cfg::GetAssetByTheme(g_Theme, "ui/UI.json")));
    const auto menu_folder_text_size = ui_json.value<u32>("menu_folder_text_size", 25);
    const auto default_font_path = ul::cfg::GetAssetByTheme(g_Theme, "ui/Font.ttf");

    auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
    renderer_opts.UseTTF(default_font_path);
    renderer_opts.UseImage(pu::ui::render::IMGAllFlags);
    renderer_opts.UseAudio(pu::ui::render::MixerAllFlags);
    renderer_opts.SetExtraDefaultFontSize(menu_folder_text_size);
    auto renderer = pu::ui::render::Renderer::New(renderer_opts);
    g_MenuApplication = ul::menu::ui::MenuApplication::New(renderer);

    g_MenuApplication->Initialize(start_mode, status, ui_json);
    g_MenuApplication->Prepare();

    // Register handlers for HOME button press detection
    ul::menu::am::RegisterLibnxLibappletHomeButtonDetection();
    ul::menu::ui::RegisterOnMessageCallback();
    ul::menu::ui::QuickMenu::RegisterHomeButtonDetection();

    // With the handlers ready, initialize uSystem message handling
    UL_RC_ASSERT(ul::menu::smi::InitializeMenuMessageHandler());

    if(start_mode == ul::smi::MenuStartMode::MenuApplicationSuspended) {
        g_MenuApplication->Show();
    }
    else {
        g_MenuApplication->ShowWithFadeIn();
    }

    // Exit RomFs manually, since we also initialized it manually
    romfsExit();

    UL_LOG_INFO("Goodbye!");

    Exit();
    return 0;
}