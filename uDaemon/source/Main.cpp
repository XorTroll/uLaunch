#include <ecs/ecs_ExternalContent.hpp>
#include <ipc/ipc_Manager.hpp>
#include <db/db_Save.hpp>
#include <os/os_Titles.hpp>
#include <os/os_HomeMenu.hpp>
#include <os/os_Account.hpp>
#include <os/os_Misc.hpp>
#include <fs/fs_Stdio.hpp>
#include <am/am_Application.hpp>
#include <am/am_LibraryApplet.hpp>
#include <am/am_HomeMenu.hpp>
#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <util/util_Convert.hpp>
#include <cfg/cfg_Config.hpp>

extern "C" {

    extern u32 __nx_applet_type;
    u32 __nx_fsdev_direntry_cache_size = 0;

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

// Note: these are placed outside of an anonymous namespace since they are accessed by IPC

ams::os::Mutex g_LastMenuMessageLock(false);
dmi::MenuMessage g_LastMenuMessage = dmi::MenuMessage::Invalid;

namespace {

    enum class UsbMode : u32 {
        Invalid,
        PlainRgba,
        Jpeg
    };

    AccountUid g_SelectedUser = {};
    hb::HbTargetParams g_HbTargetLaunchFlag = {};
    hb::HbTargetParams g_HbTargetApplicationLaunchFlag = {};
    hb::HbTargetParams g_HbTargetApplicationLaunchFlagCopy = {};
    u64 g_ApplicationLaunchFlag = 0;
    WebCommonConfig g_WebAppletLaunchFlag = {};
    bool g_AlbumAppletLaunchFlag = false;
    bool g_MenuRestartFlag = false;
    bool g_HbTargetOpenedAsApplication = false;
    bool g_AppletActive = false;
    AppletOperationMode g_OperationMode;
    u8 *g_UsbViewerBuffer = nullptr;
    u8 *g_UsbViewerReadBuffer = nullptr;
    cfg::Config g_Config = {};

    constexpr size_t UsbViewerThreadStackSize = 16_KB;
    ams::os::ThreadType g_UsbViewerThread;
    alignas(ams::os::ThreadStackAlignment) u8 g_UsbViewerThreadStack[UsbViewerThreadStackSize];
    
    UsbMode g_UsbViewerMode = UsbMode::Invalid;
    SetSysFirmwareVersion g_FwVersion = {};

    // In the USB packet, the first u32 stores the USB mode (plain RGBA or JPEG, depending on what the console supports)

    constexpr size_t UsbPacketSize = PlainRgbaScreenBufferSize + sizeof(UsbMode);

    constexpr size_t LibstratosphereHeapSize = 4_MB;
    alignas(ams::os::MemoryPageSize) constinit u8 g_LibstratosphereHeap[LibstratosphereHeapSize];

    constexpr size_t LibnxHeapSize = 4_MB;
    alignas(ams::os::MemoryPageSize) constinit u8 g_LibnxHeap[LibnxHeapSize];

}

namespace {

    dmi::DaemonStatus CreateStatus() {
        dmi::DaemonStatus status = {
            .selected_user = g_SelectedUser
        };

        memcpy(status.fw_version, g_FwVersion.display_version, sizeof(status.fw_version));

        if(am::ApplicationIsActive()) {
            if(g_HbTargetOpenedAsApplication) {
                // Homebrew
                status.params = g_HbTargetApplicationLaunchFlagCopy;
            }
            else {
                // Regular title
                status.app_id = am::ApplicationGetId();
            }
        }

        return status;
    }

    void HandleSleep() {
        appletStartSleepSequence(true);
    }

    inline Result LaunchMenu(const dmi::MenuStartMode st_mode, const dmi::DaemonStatus &status) {
        return ecs::RegisterLaunchAsApplet(am::LibraryAppletGetMenuProgramId(), static_cast<u32>(st_mode), "/ulaunch/bin/uMenu", std::addressof(status), sizeof(status));
    }

    void HandleHomeButton() {
        if(am::LibraryAppletIsActive() && !am::LibraryAppletIsMenu()) {
            // An applet is opened (which is not our menu), thus close it and reopen the menu
            am::LibraryAppletTerminate();
            UL_RC_ASSERT(LaunchMenu(dmi::MenuStartMode::Menu, CreateStatus()));
        }
        else if(am::ApplicationIsActive() && am::ApplicationHasForeground()) {
            // Hide the application currently on focus and open our menu
            am::HomeMenuSetForeground();
            UL_RC_ASSERT(LaunchMenu(dmi::MenuStartMode::MenuApplicationSuspended, CreateStatus()));
        }
        else if(am::LibraryAppletIsMenu()) {
            // Send a message to our menu to handle itself the home press
            std::scoped_lock lk(g_LastMenuMessageLock);
            g_LastMenuMessage = dmi::MenuMessage::HomeRequest;
        }
    }

    Result HandleGeneralChannel() {
        AppletStorage sams_st;
        UL_RC_TRY(appletPopFromGeneralChannel(&sams_st));
        UL_ON_SCOPE_EXIT({ appletStorageClose(&sams_st); });

        os::SystemAppletMessage sams = {};
        UL_RC_TRY(appletStorageRead(&sams_st, 0, &sams, sizeof(sams)));
        if(sams.magic == os::SystemAppletMessage::Magic) {
            switch(sams.general_channel_message) {
                // Usually this doesn't happen, HOME is detected by applet messages...?
                case os::GeneralChannelMessage::HomeButton: {
                    HandleHomeButton();
                    break;
                }
                case os::GeneralChannelMessage::Shutdown: {
                    appletStartShutdownSequence();
                    break;
                }
                case os::GeneralChannelMessage::Reboot: {
                    appletStartRebootSequence();
                    break;
                }
                case os::GeneralChannelMessage::Sleep: {
                    HandleSleep();
                    break;
                }
                // We don't have anything special to do for the rest
                default:
                    break;
            }
        }

        return ResultSuccess;
    }

    Result UpdateOperationMode() {
        // Thank you so much libnx for not exposing the actual call to get the mode via IPC :P
        // We're qlaunch, not using appletMainLoop, thus we have to take care of this manually...

        u8 tmp_mode = 0;
        UL_RC_TRY(serviceDispatchOut(appletGetServiceSession_CommonStateGetter(), 5, tmp_mode));

        g_OperationMode = static_cast<AppletOperationMode>(tmp_mode);
        return ResultSuccess;
    }

    Result HandleAppletMessage() {
        u32 raw_msg = 0;
        UL_RC_TRY(appletGetMessage(&raw_msg));

        const auto msg = static_cast<os::AppletMessage>(raw_msg);
        switch(msg) {
            case os::AppletMessage::HomeButton: {
                HandleHomeButton();
                break;
            }
            case os::AppletMessage::SdCardOut: {
                // Power off, since uMenu's UI relies on the SD card, so trying to use uMenu without the SD is quite risky...
                // TODO: handle this in a better way?
                appletStartShutdownSequence();
                break;
            }
            case os::AppletMessage::PowerButton: {
                HandleSleep();
                break;
            }
            case os::AppletMessage::ChangeOperationMode: {
                UpdateOperationMode();
                break;
            }
            default:
                break;
        }
        return ResultSuccess;
    }

    void HandleMenuMessage() {
        if(am::LibraryAppletIsMenu()) {
            char web_url[500] = {};
            u64 app_id = 0;
            hb::HbTargetParams params = {};
            dmi::dmn::ReceiveCommand([&](const dmi::DaemonMessage msg, dmi::dmn::DaemonScopedStorageReader &reader) -> Result {
                switch(msg) {
                    case dmi::DaemonMessage::SetSelectedUser: {
                        UL_RC_TRY(reader.Pop(g_SelectedUser));
                        break;
                    }
                    case dmi::DaemonMessage::LaunchApplication: {
                        UL_RC_TRY(reader.Pop(app_id));
                        break;
                    }
                    case dmi::DaemonMessage::ResumeApplication: {
                        // ...
                        break;
                    }
                    case dmi::DaemonMessage::TerminateApplication: {
                        // ...
                        break;
                    }
                    case dmi::DaemonMessage::LaunchHomebrewLibraryApplet: {
                        UL_RC_TRY(reader.Pop(g_HbTargetLaunchFlag));
                        break;
                    }
                    case dmi::DaemonMessage::LaunchHomebrewApplication: {
                        UL_RC_TRY(reader.Pop(app_id));
                        UL_RC_TRY(reader.Pop(params));
                        break;
                    }
                    case dmi::DaemonMessage::OpenWebPage: {
                        UL_RC_TRY(reader.PopData(web_url, sizeof(web_url)));
                        break;
                    }
                    case dmi::DaemonMessage::OpenAlbum: {
                        // ...
                        break;
                    }
                    case dmi::DaemonMessage::RestartMenu: {
                        // ...
                        break;
                    }
                    default: {
                        // ...
                        break;
                    }
                }
                return ResultSuccess;
            },
            [&](const dmi::DaemonMessage msg, dmi::dmn::DaemonScopedStorageWriter &writer) -> Result {
                switch(msg) {
                    case dmi::DaemonMessage::SetSelectedUser: {
                        // ...
                        break;
                    }
                    case dmi::DaemonMessage::LaunchApplication: {
                        if(am::ApplicationIsActive()) {
                            return dmn::ResultApplicationActive;
                        }
                        else if(!accountUidIsValid(&g_SelectedUser)) {
                            return dmn::ResultInvalidSelectedUser;
                        }
                        else if(g_ApplicationLaunchFlag > 0) {
                            return dmn::ResultAlreadyQueued;
                        }

                        g_ApplicationLaunchFlag = app_id;
                        break;
                    }
                    case dmi::DaemonMessage::ResumeApplication: {
                        if(!am::ApplicationIsActive()) {
                            return dmn::ResultApplicationActive;
                        }

                        am::ApplicationSetForeground();
                        break;
                    }
                    case dmi::DaemonMessage::TerminateApplication: {
                        am::ApplicationTerminate();
                        g_HbTargetOpenedAsApplication = false;
                        break;
                    }
                    case dmi::DaemonMessage::LaunchHomebrewLibraryApplet: {
                        // ...
                        break;
                    }
                    case dmi::DaemonMessage::LaunchHomebrewApplication: {
                        if(am::ApplicationIsActive()) {
                            return dmn::ResultApplicationActive;
                        }
                        else if(!accountUidIsValid(&g_SelectedUser)) {
                            return dmn::ResultInvalidSelectedUser;
                        }
                        else if(g_ApplicationLaunchFlag > 0) {
                            return dmn::ResultAlreadyQueued;
                        }
                        
                        g_HbTargetApplicationLaunchFlag = params;
                        g_HbTargetApplicationLaunchFlagCopy = params;
                        g_ApplicationLaunchFlag = app_id;
                        break;
                    }
                    case dmi::DaemonMessage::OpenWebPage: {
                        webPageCreate(&g_WebAppletLaunchFlag, web_url);
                        webConfigSetWhitelist(&g_WebAppletLaunchFlag, ".*");
                        break;
                    }
                    case dmi::DaemonMessage::OpenAlbum: {
                        g_AlbumAppletLaunchFlag = true;
                        break;
                    }
                    case dmi::DaemonMessage::RestartMenu: {
                        g_MenuRestartFlag = true;
                        break;
                    }
                    default: {
                        // ...
                        break;
                    }
                }
                return ResultSuccess;
            });
        }
    }

    void UsbViewerRgbaThread(void*) {
        while(true) {
            bool tmp_flag;
            appletGetLastForegroundCaptureImageEx(g_UsbViewerReadBuffer, PlainRgbaScreenBufferSize, &tmp_flag);
            appletUpdateLastForegroundCaptureImage();
            usbCommsWrite(g_UsbViewerBuffer, UsbPacketSize);
        }
    }

    void UsbViewerJPEGThread(void*) {
        while(true) {
            u64 tmp_size;
            capsscCaptureJpegScreenShot(&tmp_size, g_UsbViewerReadBuffer, PlainRgbaScreenBufferSize, ViLayerStack_Default, UINT64_MAX);
            usbCommsWrite(g_UsbViewerBuffer, UsbPacketSize);
        }
    }

    void PrepareUsbViewer() {
        g_UsbViewerBuffer = reinterpret_cast<u8*>(__libnx_aligned_alloc(ams::os::MemoryPageSize, UsbPacketSize));
        memset(g_UsbViewerBuffer, 0, UsbPacketSize);

        // Skip the first u32 of the buffer, since the mode is stored there
        g_UsbViewerReadBuffer = g_UsbViewerBuffer + sizeof(UsbMode);
        u64 tmp_size;
        if(R_SUCCEEDED(capsscCaptureJpegScreenShot(&tmp_size, g_UsbViewerReadBuffer, PlainRgbaScreenBufferSize, ViLayerStack_Default, UINT64_MAX))) {
            g_UsbViewerMode = UsbMode::Jpeg;
        }
        else {
            g_UsbViewerMode = UsbMode::PlainRgba;
            capsscExit();
        }
        *reinterpret_cast<UsbMode*>(g_UsbViewerBuffer) = g_UsbViewerMode;
    }

    void MainLoop() {
        HandleGeneralChannel();
        HandleAppletMessage();
        HandleMenuMessage();

        auto sth_done = false;
        // A valid version will always be >= 0x20000
        if(g_WebAppletLaunchFlag.version > 0) {
            if(!am::LibraryAppletIsActive()) {
                // TODO: applet startup sound?
                UL_RC_ASSERT(am::WebAppletStart(&g_WebAppletLaunchFlag));

                sth_done = true;
                g_WebAppletLaunchFlag = {};
            }
        }
        if(g_MenuRestartFlag) {
            if(!am::LibraryAppletIsActive()) {
                UL_RC_ASSERT(LaunchMenu(dmi::MenuStartMode::StartupScreen, CreateStatus()));

                sth_done = true;
                g_MenuRestartFlag = false;
            }
        }
        if(g_AlbumAppletLaunchFlag) {
            if(!am::LibraryAppletIsActive()) {
                const struct {
                    u8 album_arg;
                } album_data = { AlbumLaArg_ShowAllAlbumFilesForHomeMenu };
                // TODO: applet startup sound?
                UL_RC_ASSERT(am::LibraryAppletStart(AppletId_LibraryAppletPhotoViewer, 0x10000, &album_data, sizeof(album_data)));

                sth_done = true;
                g_AlbumAppletLaunchFlag = false;
            }
        }
        if(g_ApplicationLaunchFlag > 0) {
            if(!am::LibraryAppletIsActive()) {
                if(strlen(g_HbTargetApplicationLaunchFlag.nro_path)) {
                    const auto params = hb::HbTargetParams::Create(g_HbTargetApplicationLaunchFlag.nro_path, g_HbTargetApplicationLaunchFlag.nro_argv, false);
                    UL_RC_ASSERT(ecs::RegisterLaunchAsApplication(g_ApplicationLaunchFlag, "/ulaunch/bin/uHbTarget/app", &params, sizeof(params), g_SelectedUser));
                    
                    g_HbTargetOpenedAsApplication = true;
                    g_HbTargetApplicationLaunchFlag.nro_path[0] = '\0';
                }
                else {
                    UL_RC_ASSERT(am::ApplicationStart(g_ApplicationLaunchFlag, false, g_SelectedUser));
                }
                sth_done = true;
                g_ApplicationLaunchFlag = 0;
            }
        }
        if(strlen(g_HbTargetLaunchFlag.nro_path)) {
            if(!am::LibraryAppletIsActive()) {
                const auto params = hb::HbTargetParams::Create(g_HbTargetLaunchFlag.nro_path, g_HbTargetLaunchFlag.nro_argv, false);
                u64 homebrew_applet_program_id;
                UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::HomebrewAppletTakeoverProgramId, homebrew_applet_program_id));
                UL_RC_ASSERT(ecs::RegisterLaunchAsApplet(homebrew_applet_program_id, 0, "/ulaunch/bin/uHbTarget/applet", &params, sizeof(params)));
                
                sth_done = true;
                g_HbTargetLaunchFlag.nro_path[0] = '\0';
            }
        }
        if(!am::LibraryAppletIsActive()) {
            const auto cur_id = am::LibraryAppletGetId();
            u64 homebrew_applet_program_id;
            UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::HomebrewAppletTakeoverProgramId, homebrew_applet_program_id));
            if((cur_id == AppletId_LibraryAppletWeb) || (cur_id == AppletId_LibraryAppletPhotoViewer) || (cur_id == homebrew_applet_program_id)) {
                UL_RC_ASSERT(LaunchMenu(dmi::MenuStartMode::Menu, CreateStatus()));
                
                sth_done = true;
            }
        }

        const auto prev_applet_active = g_AppletActive;
        g_AppletActive = am::LibraryAppletIsActive();
        if(!sth_done && !prev_applet_active) {
            // If nothing was done but nothing is active. an application or applet might have crashed, terminated, failed to launch...
            if(!am::ApplicationIsActive() && !am::LibraryAppletIsActive()) {
                // Throw the application's result if it actually ended with a result
                auto terminate_rc = ResultSuccess;
                if(R_SUCCEEDED(nsGetApplicationTerminateResult(am::ApplicationGetId(), &terminate_rc))) {
                    UL_RC_ASSERT(terminate_rc);
                }

                // Reopen uMenu in launch-error mode
                UL_RC_ASSERT(LaunchMenu(dmi::MenuStartMode::MenuLaunchFailure, CreateStatus()));
                g_HbTargetOpenedAsApplication = false;
            }
        }

        svcSleepThread(10'000'000ul);
    }

    Result LaunchUsbViewerThread() {
        void(*thread_entry)(void*) = nullptr;
        switch(g_UsbViewerMode) {
            case UsbMode::PlainRgba: {
                thread_entry = &UsbViewerRgbaThread;
                break;
            }
            case UsbMode::Jpeg: {
                thread_entry = &UsbViewerJPEGThread;
                break;
            }
            default:
                return ResultSuccess;
        }

        UL_RC_TRY(ams::os::CreateThread(&g_UsbViewerThread, thread_entry, nullptr, g_UsbViewerThreadStack, sizeof(g_UsbViewerThreadStack), 10));
        ams::os::StartThread(&g_UsbViewerThread);

        return ResultSuccess;
    }

    void Initialize() {
        UL_RC_ASSERT(appletLoadAndApplyIdlePolicySettings());
        UpdateOperationMode();

        UL_RC_ASSERT(setsysGetFirmwareVersion(&g_FwVersion));
        
        UL_RC_ASSERT(db::Mount());

        // Remove last present error report
        fs::DeleteFile(UL_ASSERTION_LOG_FILE);

        // Remove old password files
        fs::DeleteDirectory(UL_BASE_DB_DIR "/user");
        db::Commit();

        fs::CreateDirectory(UL_BASE_DB_DIR);
        db::Commit();

        fs::CreateDirectory(UL_BASE_SD_DIR);
        fs::CreateDirectory(UL_ENTRIES_PATH);
        fs::CreateDirectory(UL_THEMES_PATH);
        fs::CreateDirectory(UL_BASE_SD_DIR "/title");
        fs::CreateDirectory(UL_BASE_SD_DIR "/user");
        fs::CreateDirectory(UL_BASE_SD_DIR "/nro");
        fs::CreateDirectory(UL_BASE_SD_DIR "/lang");

        g_Config = cfg::LoadConfig();
        u64 menu_program_id;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::MenuTakeoverProgramId, menu_program_id));
        am::LibraryAppletSetMenuAppletId(am::LibraryAppletGetAppletIdForProgramId(menu_program_id));

        bool viewer_usb_enabled;
        UL_ASSERT_TRUE(g_Config.GetEntry(cfg::ConfigEntryId::ViewerUsbEnabled, viewer_usb_enabled));
        if(viewer_usb_enabled) {
            UL_RC_ASSERT(usbCommsInitialize());
            UL_RC_ASSERT(capsscInitialize());

            PrepareUsbViewer();
            UL_RC_ASSERT(LaunchUsbViewerThread());
        }

        UL_RC_ASSERT(ipc::Initialize());
    }

    void Finalize() {
        if(g_UsbViewerBuffer != nullptr) {
            usbCommsExit();
            if(g_UsbViewerMode == UsbMode::Jpeg) {
                capsscExit();
            }
            __libnx_free(g_UsbViewerBuffer);
            g_UsbViewerBuffer = nullptr;
        }

        nsExit();
        pminfoExit();
        ldrShellExit();
        pmshellExit();
        db::Unmount();
    }

}

// TODO: consider stopping using Atmosphere-libs?

extern "C" {

    extern u8 *fake_heap_start;
    extern u8 *fake_heap_end;

}

namespace ams {

    namespace init {

        void InitializeSystemModule() {
            __nx_applet_type = AppletType_SystemApplet;
            UL_RC_ASSERT(sm::Initialize());

            UL_RC_ASSERT(appletInitialize());
            UL_RC_ASSERT(fsInitialize());
            UL_RC_ASSERT(nsInitialize());
            UL_RC_ASSERT(pminfoInitialize());
            UL_RC_ASSERT(ldrShellInitialize());
            UL_RC_ASSERT(pmshellInitialize());
            UL_RC_ASSERT(setsysInitialize());

            UL_RC_ASSERT(fsdevMountSdmc());
        }

        void FinalizeSystemModule() {}

        void Startup() {
            init::InitializeAllocator(g_LibstratosphereHeap, LibstratosphereHeapSize);

            fake_heap_start = g_LibnxHeap;
            fake_heap_end = fake_heap_start + LibnxHeapSize;

            os::SetThreadNamePointer(os::GetCurrentThread(), "ul.daemon.Main");
        }

    }

    void NORETURN Exit(int rc) {
        AMS_UNUSED(rc);
        AMS_ABORT("Exit called by qlaunch (uDaemon)");
    }

    // uDaemon handles basic qlaunch functionality and serves as a back-end for uLaunch, communicating with uMenu front-end when neccessary.

    void Main() {
        // Initialize everything
        Initialize();

        // After having initialized everything, launch our menu
        UL_RC_ASSERT(LaunchMenu(dmi::MenuStartMode::StartupScreen, CreateStatus()));

        // Loop forever, since qlaunch should NEVER terminate (AM would crash in that case)
        while(true) {
            MainLoop();
        }

        // We will never reach this anyway...
        Finalize();
    }

}