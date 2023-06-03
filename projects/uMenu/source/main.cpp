#include <ul/menu/am/am_LibnxLibappletWrap.hpp>
#include <ul/menu/am/am_LibraryAppletUtils.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
#include <ul/menu/ui/ui_Application.hpp>
#include <ul/menu/ui/ui_TransitionGuard.hpp>
// #include <ul/menu/thm/thm_Theme.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/ent/ent_Load.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_Size.hpp>

using namespace ul::util::size;

extern "C" {

    u32 __nx_applet_type = AppletType_LibraryApplet; // Explicitly declare we're a library applet (need to do so for non-hbloader homebrew)
    TimeServiceType __nx_time_service_type = TimeServiceType_System;
    u32 __nx_fs_num_sessions = 1;

    extern u8 *fake_heap_start;
    extern u8 *fake_heap_end;

    constexpr size_t HeapSize = 160_MB;

    void __libnx_initheap() {
        void *heap_addr;
        UL_RC_ASSERT(svcSetHeapSize(&heap_addr, HeapSize));

        fake_heap_start = reinterpret_cast<u8*>(heap_addr);
        fake_heap_end = fake_heap_start + HeapSize;
    }

}

ul::cfg::Config g_Config;
ul::menu::ui::Application::Ref g_Application;
ul::menu::ui::TransitionGuard g_TransitionGuard;
ul::smi::SystemStatus g_SystemStatus;
AccountUid g_SelectedUser;
ul::smi::MenuStartMode g_MenuStartMode;
// ul::menu::thm::Theme g_Theme;

namespace {

    void Initialize() {
        UL_RC_ASSERT(accountInitialize(AccountServiceType_System));
        UL_RC_ASSERT(nsInitialize());
        UL_RC_ASSERT(psmInitialize());
        UL_RC_ASSERT(setsysInitialize());
        UL_RC_ASSERT(setInitialize());
        UL_RC_ASSERT(nifmInitialize(NifmServiceType_System));
        UL_RC_ASSERT(psmInitialize());

        UL_RC_ASSERT(ul::menu::smi::InitializeSystemMessageHandler());
        ul::menu::am::RegisterLibnxLibappletHomeButtonDetection();

        UL_ASSERT_TRUE(g_Config.EnsureLoad());
        AccountUid users[ACC_USER_LIST_SIZE];
        s32 count;
        UL_RC_ASSERT(accountListAllUsers(users, ACC_USER_LIST_SIZE, &count));

        g_SelectedUser = users[0];
        UL_RC_ASSERT(ul::menu::smi::SetSelectedUser(g_SelectedUser));

        /*
        const auto themes = ul::menu::thm::LoadThemes();
        for(const auto &theme : themes) {
            if(theme.manifest.name == g_Config.active_theme_name) {
                g_Theme = theme;
                break;
            }
        }

        UL_ASSERT_TRUE(g_Theme.IsValid());
        */
    }

    NORETURN void Finalize() {
        ul::menu::smi::FinalizeSystemMessageHandler();

        psmExit();
        nifmExit();
        setExit();
        setsysExit();
        psmExit();
        nsExit();
        accountExit();
        exit(0);
        __builtin_unreachable();
    }

}

int main() {
    UL_RC_ASSERT(ul::menu::am::ReadStartMode(g_MenuStartMode));
    UL_ASSERT_TRUE(g_MenuStartMode != ul::smi::MenuStartMode::Invalid);

    // Information sent as an extra storage to us
    UL_RC_ASSERT(ul::menu::am::ReadFromInputStorage(&g_SystemStatus, sizeof(g_SystemStatus)));

    Initialize();

    auto renderer_opts = pu::ui::render::RendererInitOptions(SDL_INIT_EVERYTHING, pu::ui::render::RendererHardwareFlags);
    renderer_opts.UseTTF();
    renderer_opts.UseImage(pu::ui::render::IMGAllFlags);
    renderer_opts.UseAudio(pu::ui::render::MixerAllFlags);
    auto renderer = pu::ui::render::Renderer::New(renderer_opts);
    g_Application = ul::menu::ui::Application::New(renderer);

    g_Application->Prepare();

    // Register handlers for different event detections
    ul::menu::ui::Application::RegisterHomeButtonDetection();
    ul::menu::ui::Application::RegisterSdCardEjectedDetection();
    ul::menu::ui::QuickMenu::RegisterHomeButtonDetection();

    if(g_MenuStartMode == ul::smi::MenuStartMode::MenuApplicationSuspended) {
        g_Application->Show();
    }
    else {
        g_Application->ShowWithFadeIn();
    }
    
    Finalize();
}