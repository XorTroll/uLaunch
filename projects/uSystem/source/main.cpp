#include <ul/system/ecs/ecs_ExternalContent.hpp>
#include <ul/system/sf/sf_IpcManager.hpp>
#include <ul/system/smi/smi_SystemProtocol.hpp>
#include <ul/system/sys/sys_SystemApplet.hpp>
#include <ul/system/system_Message.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/util/util_Size.hpp>
#include <queue>

extern "C" {

    extern u32 __nx_applet_type;

    // So that libstratosphere doesn't redefine them as invalid

    void *__libnx_alloc(size_t size) {
        return malloc(size);
    }

    void *__libnx_aligned_alloc(size_t align, size_t size) {
        return aligned_alloc(align, size);
    }

    void __libnx_free(void *ptr) {
        return free(ptr);
    }

}

using namespace ul::util::size;
using namespace ul::system;

// Note: these are global since they are accessed by IPC

ams::os::Mutex g_MenuMessageQueueLock(false);
std::queue<ul::smi::MenuMessage> *g_MenuMessageQueue;

#define DEBUG_LOG(fmt, ...) ({ \
    auto f = fopen("sdmc:/ulaunch/usystem-tmp.log", "ab+"); \
    if(f) { \
        fprintf(f, fmt "\n", ##__VA_ARGS__); \
        fclose(f); \
    } \
})

namespace {

    AccountUid g_SelectedUser = {};
    ul::loader::TargetInput g_LoaderLaunchFlag = {};
    ul::loader::TargetInput g_LoaderApplicationLaunchFlag = {};
    ul::loader::TargetInput g_LoaderApplicationLaunchFlagCopy = {};
    u64 g_ApplicationLaunchFlag = 0;
    WebCommonConfig g_WebAppletLaunchFlag = {};
    bool g_AlbumAppletLaunchFlag = false;
    bool g_MenuRestartFlag = false;
    bool g_LoaderOpenedAsApplication = false;
    bool g_AppletActive = false;
    AppletOperationMode g_OperationMode;
    ul::cfg::Config g_Config = {};
    
    SetSysFirmwareVersion g_FwVersion = {};

    constexpr size_t LibstratosphereHeapSize = 4_MB;
    alignas(ams::os::MemoryPageSize) constinit u8 g_LibstratosphereHeap[LibstratosphereHeapSize];

    constexpr size_t LibnxHeapSize = 4_MB;
    alignas(ams::os::MemoryPageSize) constinit u8 g_LibnxHeap[LibnxHeapSize];

}

namespace {

    inline void UpdateLastMenuMessage(const ul::smi::MenuMessage msg) {
        std::scoped_lock lk(g_MenuMessageQueueLock);
        g_MenuMessageQueue->push(msg);
    }

    ul::smi::SystemStatus CreateStatus() {
        ul::smi::SystemStatus status = {
            .selected_user = g_SelectedUser
        };

        memcpy(status.fw_version, g_FwVersion.display_version, sizeof(status.fw_version));

        if(app::IsActive()) {
            if(g_LoaderOpenedAsApplication) {
                // Homebrew
                status.suspended_hb_target_ipt = g_LoaderApplicationLaunchFlagCopy;
            }
            else {
                // Regular title
                status.suspended_app_id = app::GetId();
            }
        }

        return status;
    }

    void HandleSleep() {
        appletStartSleepSequence(true);
    }

    inline Result LaunchMenu(const ul::smi::MenuStartMode st_mode, const ul::smi::SystemStatus &status) {
        return ecs::RegisterLaunchAsApplet(la::GetMenuProgramId(), static_cast<u32>(st_mode), "/ulaunch/bin/uMenu", std::addressof(status), sizeof(status));
    }

    void HandleHomeButton() {
        if(la::IsActive() && !la::IsMenu()) {
            // An applet is opened (which is not our menu), thus close it and reopen the menu
            la::Terminate();
            UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::Menu, CreateStatus()));
        }
        else if(app::IsActive() && app::HasForeground()) {
            // Hide the application currently on focus and open our menu
            sys::SetForeground();
            UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::MenuApplicationSuspended, CreateStatus()));
        }
        else if(la::IsMenu()) {
            // Send a message to our menu to handle itself the home press
            UpdateLastMenuMessage(ul::smi::MenuMessage::HomeRequest);
        }
    }

    void HandleGeneralChannel() {
        AppletStorage sams_st;
        if(R_SUCCEEDED(appletPopFromGeneralChannel(&sams_st))) {
            ul::util::OnScopeExit close_sams_st([&]() {
                appletStorageClose(&sams_st);
            });

            StorageReader sams_st_reader(sams_st);

            // Note: are we expecting a certain size? (sometimes we get 0x10, others 0x800...)
            const auto sams_st_size = sams_st_reader.GetSize();

            SystemAppletMessageHeader sams_header;
            UL_RC_ASSERT(sams_st_reader.Read(sams_header));
            DEBUG_LOG("SystemAppletMessageHeader [size: 0x%lX] { magic: 0x%X, unk: 0x%X, msg: %d, unk_2: 0x%X }", sams_st_size, sams_header.magic, sams_header.unk, static_cast<u32>(sams_header.msg), sams_header.unk_2);

            if(sams_header.IsValid()) {
                switch(sams_header.msg) {
                    case GeneralChannelMessage::Unk_Invalid: {
                        DEBUG_LOG("Invalid general channel message!");
                        break;
                    }
                    case GeneralChannelMessage::RequestHomeMenu: {
                        HandleHomeButton();
                        break;
                    }
                    case GeneralChannelMessage::Unk_Sleep: {
                        HandleSleep();
                        break;
                    }
                    case GeneralChannelMessage::Unk_Shutdown: {
                        UL_RC_ASSERT(appletStartShutdownSequence());
                        break;
                    }
                    case GeneralChannelMessage::Unk_Reboot: {
                        UL_RC_ASSERT(appletStartRebootSequence());
                        break;
                    }
                    case GeneralChannelMessage::RequestJumpToSystemUpdate: {
                        DEBUG_LOG("Unimplemented: RequestJumpToSystemUpdate");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayBrightValueChanged: {
                        DEBUG_LOG("Unimplemented: Unk_OverlayBrightValueChanged");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayAutoBrightnessChanged: {
                        DEBUG_LOG("Unimplemented: Unk_OverlayAutoBrightnessChanged");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayAirplaneModeChanged: {
                        DEBUG_LOG("Unimplemented: Unk_OverlayAirplaneModeChanged");
                        break;
                    }
                    case GeneralChannelMessage::Unk_HomeButtonHold: {
                        DEBUG_LOG("Unimplemented: Unk_HomeButtonHold");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayHidden: {
                        DEBUG_LOG("Unimplemented: Unk_OverlayHidden");
                        break;
                    }
                    case GeneralChannelMessage::RequestToLaunchApplication: {
                        // TODO: enum?
                        u32 launch_app_request_sender;
                        UL_RC_ASSERT(sams_st_reader.Read(launch_app_request_sender));

                        u64 app_id;
                        UL_RC_ASSERT(sams_st_reader.Read(app_id));

                        AccountUid uid;
                        UL_RC_ASSERT(sams_st_reader.Read(uid));

                        u32 launch_params_buf_size;
                        UL_RC_ASSERT(sams_st_reader.Read(launch_params_buf_size));

                        auto launch_params_buf = new u8[launch_params_buf_size];
                        UL_RC_ASSERT(sams_st_reader.ReadBuffer(launch_params_buf, launch_params_buf_size));

                        DEBUG_LOG("Unimplemented: RequestToLaunchApplication { launch_app_request_sender: %d, app_id: 0%16lX, uid: %016lX + %016lX, launch params buf size: 0x%X }", launch_app_request_sender, app_id, uid.uid[0], uid.uid[1], launch_params_buf_size);

                        delete[] launch_params_buf;
                        break;
                    }
                    case GeneralChannelMessage::RequestJumpToStory: {
                        AccountUid uid;
                        UL_RC_ASSERT(sams_st_reader.Read(uid));

                        u64 app_id;
                        UL_RC_ASSERT(sams_st_reader.Read(app_id));

                        DEBUG_LOG("Unimplemented: RequestJumpToStory { uid: %016lX + %016lX, app_id: 0%16lX }", uid.uid[0], uid.uid[1], app_id);
                        break;
                    }
                    default:
                        // TODO
                        DEBUG_LOG("Unhandled general channel message!");
                        break;
                }
            }
        }
    }

    Result UpdateOperationMode() {
        // Thank you so much libnx for not exposing the actual call to get the mode via IPC :P
        // We're qlaunch, not using appletMainLoop, thus we have to take care of this manually...

        u8 raw_mode = 0;
        UL_RC_TRY(serviceDispatchOut(appletGetServiceSession_CommonStateGetter(), 5, raw_mode));

        g_OperationMode = static_cast<AppletOperationMode>(raw_mode);
        return ul::ResultSuccess;
    }

    void HandleAppletMessage() {
        u32 raw_msg = 0;
        if(R_SUCCEEDED(appletGetMessage(&raw_msg))) {
            DEBUG_LOG("AppletMessage: %d", raw_msg);

            /*
            Applet messages known to be received by us:
            - ChangeIntoForeground
            - ChangeIntoBackground
            - ApplicationExited
            - DetectShortPressingHomeButton
            - DetectShortPressingPowerButton
            - FinishedSleepSequence
            - OperationModeChanged
            - SdCardRemoved
            */

            switch(static_cast<ul::system::AppletMessage>(raw_msg)) {
                case ul::system::AppletMessage::DetectShortPressingHomeButton: {
                    HandleHomeButton();
                    break;
                }
                case ul::system::AppletMessage::SdCardRemoved: {
                    // Power off, since uMenu's UI relies on the SD card, so trying to use uMenu without the SD is quite risky...
                    // TODO: handle this in a better way?
                    if(la::IsMenu()) {
                        UpdateLastMenuMessage(ul::smi::MenuMessage::SdCardEjected);
                    }
                    else {
                        UL_RC_ASSERT(appletStartShutdownSequence());
                    }
                    break;
                }
                case ul::system::AppletMessage::DetectShortPressingPowerButton: {
                    HandleSleep();
                    break;
                }
                case ul::system::AppletMessage::OperationModeChanged: {
                    UL_RC_ASSERT(UpdateOperationMode());
                    break;
                }
                default:
                    // TODO: more?
                    break;
            }
        } 
    }

    void HandleMenuMessage() {
        if(la::IsMenu()) {
            // Note: ignoring result since this won't always work, and would error if no commands were received
            smi::ReceiveCommand(
                [&](const ul::smi::SystemMessage msg, smi::ScopedStorageReader &reader) -> Result {
                    switch(msg) {
                        case ul::smi::SystemMessage::SetSelectedUser: {
                            UL_RC_TRY(reader.Pop(g_SelectedUser));
                            break;
                        }
                        case ul::smi::SystemMessage::LaunchApplication: {
                            u64 launch_app_id;
                            UL_RC_TRY(reader.Pop(launch_app_id));

                            if(app::IsActive()) {
                                return smi::ResultApplicationActive;
                            }
                            if(!accountUidIsValid(&g_SelectedUser)) {
                                return smi::ResultInvalidSelectedUser;
                            }
                            if(g_ApplicationLaunchFlag > 0) {
                                return smi::ResultAlreadyQueued;
                            }

                            g_ApplicationLaunchFlag = launch_app_id;
                            break;
                        }
                        case ul::smi::SystemMessage::ResumeApplication: {
                            if(!app::IsActive()) {
                                return smi::ResultApplicationActive;
                            }

                            UL_RC_TRY(app::SetForeground());
                            break;
                        }
                        case ul::smi::SystemMessage::TerminateApplication: {
                            app::Terminate();
                            g_LoaderOpenedAsApplication = false;
                            break;
                        }
                        case ul::smi::SystemMessage::LaunchHomebrewLibraryApplet: {
                            UL_RC_TRY(reader.Pop(g_LoaderLaunchFlag));
                            break;
                        }
                        case ul::smi::SystemMessage::LaunchHomebrewApplication: {
                            ul::loader::TargetInput temp_ipt;
                            UL_RC_TRY(reader.Pop(temp_ipt));

                            if(app::IsActive()) {
                                return smi::ResultApplicationActive;
                            }
                            if(!accountUidIsValid(&g_SelectedUser)) {
                                return smi::ResultInvalidSelectedUser;
                            }
                            if(g_ApplicationLaunchFlag > 0) {
                                return smi::ResultAlreadyQueued;
                            }

                            u64 hb_application_takeover_program_id;
                            UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, hb_application_takeover_program_id));
                            if(hb_application_takeover_program_id == 0) {
                                return smi::ResultNoHomebrewTakeoverApplication;
                            }

                            g_LoaderApplicationLaunchFlag = temp_ipt;
                            g_LoaderApplicationLaunchFlagCopy = temp_ipt;

                            g_ApplicationLaunchFlag = hb_application_takeover_program_id;
                            break;
                        }
                        case ul::smi::SystemMessage::OpenWebPage: {
                            char web_url[500] = {};
                            UL_RC_TRY(reader.PopData(web_url, sizeof(web_url)));

                            UL_RC_TRY(webPageCreate(&g_WebAppletLaunchFlag, web_url));
                            UL_RC_TRY(webConfigSetWhitelist(&g_WebAppletLaunchFlag, ".*"));
                            break;
                        }
                        case ul::smi::SystemMessage::OpenAlbum: {
                            g_AlbumAppletLaunchFlag = true;
                            break;
                        }
                        case ul::smi::SystemMessage::RestartMenu: {
                            g_MenuRestartFlag = true;
                            break;
                        }
                        case ul::smi::SystemMessage::SetHomebrewTakeoverApplication: {
                            u64 takeover_app_id;
                            UL_RC_TRY(reader.Pop(takeover_app_id));

                            UL_ASSERT_TRUE(g_Config.SetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, takeover_app_id));
                            ul::cfg::SaveConfig(g_Config);
                            break;
                        }
                        default: {
                            // ...
                            break;
                        }
                    }
                    return ul::ResultSuccess;
                },
                [&](const ul::smi::SystemMessage msg, smi::ScopedStorageWriter &writer) -> Result {
                    AMS_UNUSED(writer);
                    switch(msg) {
                        case ul::smi::SystemMessage::SetSelectedUser: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::LaunchApplication: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::ResumeApplication: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::TerminateApplication: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::LaunchHomebrewLibraryApplet: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::LaunchHomebrewApplication: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::OpenWebPage: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::OpenAlbum: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::RestartMenu: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::SetHomebrewTakeoverApplication: {
                            // ...
                            break;
                        }
                        default: {
                            // ...
                            break;
                        }
                    }
                    return ul::ResultSuccess;
                }
            );
        }
    }

    void MainLoop() {
        HandleGeneralChannel();
        HandleAppletMessage();
        HandleMenuMessage();

        auto sth_done = false;
        // A valid version will always be >= 0x20000
        if(g_WebAppletLaunchFlag.version > 0) {
            if(!la::IsActive()) {
                // TODO: applet startup sound?
                UL_RC_ASSERT(la::StartWeb(&g_WebAppletLaunchFlag));

                sth_done = true;
                g_WebAppletLaunchFlag = {};
            }
        }
        if(g_MenuRestartFlag) {
            if(!la::IsActive()) {
                UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::StartupScreen, CreateStatus()));

                sth_done = true;
                g_MenuRestartFlag = false;
            }
        }
        if(g_AlbumAppletLaunchFlag) {
            if(!la::IsActive()) {
                const struct {
                    u8 album_arg;
                } album_data = { AlbumLaArg_ShowAllAlbumFilesForHomeMenu };
                // TODO: applet startup sound?
                UL_RC_ASSERT(la::Start(AppletId_LibraryAppletPhotoViewer, 0x10000, &album_data, sizeof(album_data)));

                sth_done = true;
                g_AlbumAppletLaunchFlag = false;
            }
        }
        if(g_ApplicationLaunchFlag > 0) {
            if(!la::IsActive()) {
                // Ensure the application is launchable
                UL_RC_ASSERT(nsTouchApplication(g_ApplicationLaunchFlag));

                if(g_LoaderApplicationLaunchFlag.magic == ul::loader::TargetInput::Magic) {
                    UL_RC_ASSERT(ecs::RegisterLaunchAsApplication(g_ApplicationLaunchFlag, "/umad/bin/uLoader/application", &g_LoaderApplicationLaunchFlag, sizeof(g_LoaderApplicationLaunchFlag), g_SelectedUser));

                    g_LoaderOpenedAsApplication = true;
                    g_LoaderApplicationLaunchFlag = {};
                }
                else {
                    UL_RC_ASSERT(app::Start(g_ApplicationLaunchFlag, false, g_SelectedUser));
                }

                sth_done = true;
                g_ApplicationLaunchFlag = 0;
            }
        }
        if(g_LoaderLaunchFlag.magic == ul::loader::TargetInput::Magic) {
            if(!la::IsActive()) {
                u64 hb_applet_takeover_program_id;
                UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::HomebrewAppletTakeoverProgramId, hb_applet_takeover_program_id));
                UL_RC_ASSERT(ecs::RegisterLaunchAsApplet(hb_applet_takeover_program_id, 0, "/ulaunch/bin/uLoader/applet", &g_LoaderLaunchFlag, sizeof(g_LoaderLaunchFlag)));
                
                sth_done = true;
                g_LoaderLaunchFlag = {};
            }
        }
        if(!la::IsActive()) {
            const auto cur_id = la::GetLastAppletId();
            u64 hb_applet_takeover_program_id;
            UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::HomebrewAppletTakeoverProgramId, hb_applet_takeover_program_id));
            if((cur_id == AppletId_LibraryAppletWeb) || (cur_id == AppletId_LibraryAppletPhotoViewer) || (cur_id == hb_applet_takeover_program_id)) {
                UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::Menu, CreateStatus()));
                
                sth_done = true;
            }
        }

        const auto prev_applet_active = g_AppletActive;
        g_AppletActive = la::IsActive();
        if(!sth_done && !prev_applet_active) {
            // If nothing was done but nothing is active, an application or applet might have crashed, terminated, failed to launch...
            if(!app::IsActive() && !la::IsActive()) {
                // Throw the application's result if it actually ended with a result
                auto terminate_rc = ul::ResultSuccess;
                if(R_SUCCEEDED(nsGetApplicationTerminateResult(app::GetId(), &terminate_rc))) {
                    UL_RC_ASSERT(terminate_rc);
                }

                // Reopen uMenu in launch-error mode
                UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::MenuLaunchFailure, CreateStatus()));
                g_LoaderOpenedAsApplication = false;
            }
        }

        svcSleepThread(10'000'000ul);
    }

    void Initialize() {
        UL_RC_ASSERT(appletLoadAndApplyIdlePolicySettings());
        UL_RC_ASSERT(UpdateOperationMode());

        UL_RC_ASSERT(setsysGetFirmwareVersion(&g_FwVersion));

        // ul::fs::CreateDirectory(ul::ent::RootDirectory);
        // ul::fs::CreateDirectory(ul::ent::CacheDirectory);

        // ul::ent::CacheEnsureEntries();

        g_Config = ul::cfg::LoadConfig();
        u64 menu_program_id;
        UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::MenuTakeoverProgramId, menu_program_id));
        la::SetMenuProgramId(menu_program_id);

        UL_RC_ASSERT(sf::Initialize());
    }

}

extern "C" {

    extern u8 *fake_heap_start;
    extern u8 *fake_heap_end;

}

// TODO: stop using libstratosphere?

namespace ams {

    namespace init {

        void InitializeSystemModule() {
            UL_RC_ASSERT(sm::Initialize());

            __nx_applet_type = AppletType_SystemApplet;
            UL_RC_ASSERT(appletInitialize());
            UL_RC_ASSERT(fsInitialize());
            UL_RC_ASSERT(nsInitialize());
            UL_RC_ASSERT(pminfoInitialize());
            UL_RC_ASSERT(ldrShellInitialize());
            UL_RC_ASSERT(pmshellInitialize());
            UL_RC_ASSERT(setsysInitialize());

            UL_RC_ASSERT(fsdevMountSdmc());
        }

        void FinalizeSystemModule() {
            fsdevUnmountAll();
            setsysExit();
            pmshellExit();
            pminfoExit();
            nsExit();
            fsExit();
            appletExit();
        }

        void Startup() {
            // Initialize the global malloc-free/new-delete allocator
            init::InitializeAllocator(g_LibstratosphereHeap, LibstratosphereHeapSize);

            fake_heap_start = g_LibnxHeap;
            fake_heap_end = fake_heap_start + LibnxHeapSize;

            g_MenuMessageQueue = new std::queue<ul::smi::MenuMessage>();

            os::SetThreadNamePointer(os::GetCurrentThread(), "ul.system.Main");
        }

    }

    NORETURN void Exit(int rc) {
        AMS_UNUSED(rc);
        AMS_ABORT("Unexpected exit called by system applet (uSystem)");
    }

    // uSystem handles basic qlaunch functionality since it is the back-end of the project, communicating with uMenu when neccessary

    void Main() {
        // Initialize everything
        Initialize();

        // After having initialized everything, launch our menu
        UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::StartupScreen, CreateStatus()));

        // Loop forever, since qlaunch should NEVER terminate (AM would crash in that case)
        while(true) {
            MainLoop();
        }
    }

}