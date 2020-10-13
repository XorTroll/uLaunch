#include <ecs/ecs_ExternalContent.hpp>
#include <ipc/ipc_GlobalManager.hpp>
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

// Outside of anonymous namespace since these are accessed by IPC

ams::os::Mutex g_LastMenuMessageLock(false);
dmi::MenuMessage g_LastMenuMessage = dmi::MenuMessage::Invalid;

namespace {

    // Heap size of 8MB
    constexpr size_t HeapSize = 0x800000;
    u8 g_Heap[HeapSize];

    enum class UsbMode : u32 {
        Invalid,
        RawRGBA,
        JPEG
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
    AppletOperationMode g_OperationMode;
    u8 *g_UsbViewerBuffer = nullptr;
    u8 *g_UsbViewerReadBuffer = nullptr;
    cfg::Config g_Config = {};
    ams::os::ThreadType g_UsbViewerThread;
    alignas(ams::os::ThreadStackAlignment) u8 g_UsbViewerThreadStack[0x4000];
    UsbMode g_UsbViewerMode = UsbMode::Invalid;

    // In the USB packet, the first u32 stores the USB mode (raw RGBA or JPEG, depending on what the console supports)

    constexpr size_t UsbPacketSize = RawRGBAScreenBufferSize + sizeof(UsbMode);

}

// Needed by libstratosphere

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemAppletId::Qlaunch;
    
    namespace result {

        bool CallFatalOnResultAssertion = true;

    }

}

extern "C" {

    u32 __nx_applet_type = AppletType_SystemApplet;
    u32 __nx_fs_num_sessions = 1;
    bool __nx_fsdev_support_cwd = false;
    u32 __nx_fsdev_direntry_cache_size = 0;

    void __libnx_initheap();
    void __appInit();
    void __appExit();

}

extern char *fake_heap_start;
extern char *fake_heap_end;

void __libnx_initheap() {
    fake_heap_start = reinterpret_cast<char*>(g_Heap);
    fake_heap_end = fake_heap_start + HeapSize;
}

void __appInit() {
    ams::hos::InitializeForStratosphere();

    ams::sm::DoWithSession([]() {
        UL_ASSERT(appletInitialize());
        UL_ASSERT(fsInitialize());
        UL_ASSERT(nsInitialize());
        UL_ASSERT(pminfoInitialize());
        UL_ASSERT(ldrShellInitialize());
        UL_ASSERT(pmshellInitialize());
    });

    fsdevMountSdmc();
}

void __appExit() {
    // qlaunch should not terminate, so this is considered an invalid system state
    // am would fatal otherwise
    fatalThrow(0xDEADBABE);
}

namespace {

    dmi::DaemonStatus CreateStatus() {
        dmi::DaemonStatus status = {};
        status.selected_user = g_SelectedUser;

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

    inline Result LaunchMenu(dmi::MenuStartMode stmode, dmi::DaemonStatus status) {
        return ecs::RegisterLaunchAsApplet(g_Config.menu_program_id, static_cast<u32>(stmode), "/ulaunch/bin/uMenu", &status, sizeof(status));
    }

    void HandleHomeButton() {
        if(am::LibraryAppletIsActive() && !am::LibraryAppletIsMenu()) {
            // An applet is opened (which is not our menu), thus close it and reopen the menu
            am::LibraryAppletTerminate();
            auto status = CreateStatus();
            UL_ASSERT(LaunchMenu(dmi::MenuStartMode::Menu, status));
        }
        else if(am::ApplicationIsActive() && am::ApplicationHasForeground()) {
            // Hide the application currently on focus and open our menu
            am::HomeMenuSetForeground();
            auto status = CreateStatus();
            UL_ASSERT(LaunchMenu(dmi::MenuStartMode::MenuApplicationSuspended, status));
        }
        else if(am::LibraryAppletIsMenu()) {
            // Send a message to our menu to handle itself the home press
            std::scoped_lock lk(g_LastMenuMessageLock);
            g_LastMenuMessage = dmi::MenuMessage::HomeRequest;
        }
    }

    Result HandleGeneralChannel() {
        AppletStorage sams_st;
        R_TRY(appletPopFromGeneralChannel(&sams_st));
        UL_ON_SCOPE_EXIT({
            appletStorageClose(&sams_st);
        });

        os::SystemAppletMessage sams = {};
        R_TRY(appletStorageRead(&sams_st, 0, &sams, sizeof(sams)));
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
        R_TRY(serviceDispatchOut(appletGetServiceSession_CommonStateGetter(), 5, tmp_mode));

        g_OperationMode = static_cast<AppletOperationMode>(tmp_mode);
        return ResultSuccess;
    }

    Result HandleAppletMessage() {
        u32 raw_msg = 0;
        R_TRY(appletGetMessage(&raw_msg));

        auto msg = static_cast<os::AppletMessage>(raw_msg);
        switch(msg) {
            case os::AppletMessage::HomeButton: {
                HandleHomeButton();
                break;
            }
            case os::AppletMessage::SdCardOut: {
                // Power off, since uMenu's UI relies on the SD card, so trying to use uMenu without the SD is quite risky...
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
            char web_url[500] = {0};
            u64 app_id = 0;
            hb::HbTargetParams ipt = {};
            dmi::daemon::ReceiveCommand([&](dmi::DaemonMessage msg, dmi::daemon::DaemonScopedStorageReader &reader) -> Result {
                switch(msg) {
                    case dmi::DaemonMessage::SetSelectedUser: {
                        R_TRY(reader.Pop(g_SelectedUser));
                        break;
                    }
                    case dmi::DaemonMessage::LaunchApplication: {
                        R_TRY(reader.Pop(app_id));
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
                        R_TRY(reader.Pop(g_HbTargetLaunchFlag));
                        break;
                    }
                    case dmi::DaemonMessage::LaunchHomebrewApplication: {
                        R_TRY(reader.Pop(app_id));
                        R_TRY(reader.Pop(ipt));
                        break;
                    }
                    case dmi::DaemonMessage::OpenWebPage: {
                        R_TRY(reader.PopData(web_url, sizeof(web_url)));
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
            [&](dmi::DaemonMessage msg, dmi::daemon::DaemonScopedStorageWriter &writer) -> Result {
                switch(msg) {
                    case dmi::DaemonMessage::SetSelectedUser: {
                        // ...
                        break;
                    }
                    case dmi::DaemonMessage::LaunchApplication: {
                        if(am::ApplicationIsActive()) {
                            return RES_VALUE(Daemon, ApplicationActive);
                        }
                        else if(!accountUidIsValid(&g_SelectedUser)) {
                            return RES_VALUE(Daemon, InvalidSelectedUser);
                        }
                        else if(g_ApplicationLaunchFlag > 0) {
                            return RES_VALUE(Daemon, AlreadyQueued);
                        }

                        g_ApplicationLaunchFlag = app_id;
                        break;
                    }
                    case dmi::DaemonMessage::ResumeApplication: {
                        if(!am::ApplicationIsActive()) {
                            return RES_VALUE(Daemon, ApplicationNotActive);
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
                            return RES_VALUE(Daemon, ApplicationActive);
                        }
                        else if(!accountUidIsValid(&g_SelectedUser)) {
                            return RES_VALUE(Daemon, InvalidSelectedUser);
                        }
                        else if(g_ApplicationLaunchFlag > 0) {
                            return RES_VALUE(Daemon, AlreadyQueued);
                        }
                        
                        g_HbTargetApplicationLaunchFlag = ipt;
                        g_HbTargetApplicationLaunchFlagCopy = ipt;
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

    void UsbViewerRGBAThread(void *arg) {
        while(true) {
            bool tmp_flag;
            appletGetLastForegroundCaptureImageEx(g_UsbViewerReadBuffer, RawRGBAScreenBufferSize, &tmp_flag);
            appletUpdateLastForegroundCaptureImage();
            usbCommsWrite(g_UsbViewerBuffer, UsbPacketSize);
        }
    }

    void UsbViewerJPEGThread(void *arg) {
        while(true) {
            u64 tmp_size;
            capsscCaptureJpegScreenShot(&tmp_size, g_UsbViewerReadBuffer, RawRGBAScreenBufferSize, ViLayerStack_Default, UINT64_MAX);
            usbCommsWrite(g_UsbViewerBuffer, UsbPacketSize);
        }
    }

    void PrepareUsbViewer() {
        g_UsbViewerBuffer = new (std::align_val_t(0x1000)) u8[UsbPacketSize]();
        // Skip the first u32 of the buffer, since the mode is stored there
        g_UsbViewerReadBuffer = g_UsbViewerBuffer + sizeof(UsbMode);
        u64 tmp_size;
        if(R_SUCCEEDED(capsscCaptureJpegScreenShot(&tmp_size, g_UsbViewerReadBuffer, RawRGBAScreenBufferSize, ViLayerStack_Default, UINT64_MAX))) {
            g_UsbViewerMode = UsbMode::JPEG;
        }
        else {
            g_UsbViewerMode = UsbMode::RawRGBA;
            capsscExit();
        }
        *reinterpret_cast<UsbMode*>(g_UsbViewerBuffer) = g_UsbViewerMode;
    }

    void MainLoop() {
        HandleGeneralChannel();
        HandleAppletMessage();
        HandleMenuMessage();

        bool sth_done = false;
        // A valid version in this g_Config is always >= 0x20000
        if(g_WebAppletLaunchFlag.version > 0) {
            if(!am::LibraryAppletIsActive()) {
                UL_ASSERT(am::WebAppletStart(&g_WebAppletLaunchFlag));

                sth_done = true;
                g_WebAppletLaunchFlag = {};
            }
        }
        if(g_MenuRestartFlag) {
            if(!am::LibraryAppletIsActive()) {
                auto status = CreateStatus();
                UL_ASSERT(LaunchMenu(dmi::MenuStartMode::StartupScreen, status));

                sth_done = true;
                g_MenuRestartFlag = false;
            }
        }
        if(g_AlbumAppletLaunchFlag) {
            if(!am::LibraryAppletIsActive()) {
                u8 albumflag = 2;
                UL_ASSERT(am::LibraryAppletStart(AppletId_photoViewer, 0x10000, &albumflag, sizeof(albumflag)));

                sth_done = true;
                g_AlbumAppletLaunchFlag = false;
            }
        }
        if(g_ApplicationLaunchFlag > 0) {
            if(!am::LibraryAppletIsActive()) {
                if(strlen(g_HbTargetApplicationLaunchFlag.nro_path)) {
                    auto params = hb::HbTargetParams::Create(g_HbTargetApplicationLaunchFlag.nro_path, g_HbTargetApplicationLaunchFlag.nro_argv, false);
                    UL_ASSERT(ecs::RegisterLaunchAsApplication(g_ApplicationLaunchFlag, "/ulaunch/bin/uHbTarget/app", &params, sizeof(params), g_SelectedUser));
                    
                    g_HbTargetOpenedAsApplication = true;
                    g_HbTargetApplicationLaunchFlag.nro_path[0] = '\0';
                }
                else {
                    UL_ASSERT(am::ApplicationStart(g_ApplicationLaunchFlag, false, g_SelectedUser));
                }
                sth_done = true;
                g_ApplicationLaunchFlag = 0;
            }
        }
        if(strlen(g_HbTargetLaunchFlag.nro_path)) {
            if(!am::LibraryAppletIsActive()) {
                auto params = hb::HbTargetParams::Create(g_HbTargetLaunchFlag.nro_path, g_HbTargetLaunchFlag.nro_argv, false);
                UL_ASSERT(ecs::RegisterLaunchAsApplet(g_Config.homebrew_applet_program_id, 0, "/ulaunch/bin/uHbTarget/applet", &params, sizeof(params)));
                
                sth_done = true;
                g_HbTargetLaunchFlag.nro_path[0] = '\0';
            }
        }
        if(!am::LibraryAppletIsActive()) {
            auto cur_id = am::LibraryAppletGetId();
            if((cur_id == AppletId_web) || (cur_id == AppletId_photoViewer) || (cur_id == g_Config.homebrew_applet_program_id)) {
                auto status = CreateStatus();
                UL_ASSERT(LaunchMenu(dmi::MenuStartMode::Menu, status));
                
                sth_done = true;
            }
        }
        if(!sth_done) {
            // If nothing was done, but nothing is active... An application or applet might have crashed, terminated, failed to launch...
            // No matter what is it, we reopen Menu in launch-error mode.
            if(!am::ApplicationIsActive() && !am::LibraryAppletIsActive()) {
                auto status = CreateStatus();
                UL_ASSERT(LaunchMenu(dmi::MenuStartMode::MenuLaunchFailure, status));
            }
        }
        svcSleepThread(10'000'000ul);
    }

    Result LaunchUsbViewerThread() {
        void(*thread_entry)(void*) = nullptr;
        switch(g_UsbViewerMode) {
            case UsbMode::RawRGBA: {
                thread_entry = &UsbViewerRGBAThread;
                break;
            }
            case UsbMode::JPEG: {
                thread_entry = &UsbViewerJPEGThread;
                break;
            }
            default:
                return ResultSuccess;
        }

        R_TRY(ams::os::CreateThread(&g_UsbViewerThread, thread_entry, nullptr, g_UsbViewerThreadStack, sizeof(g_UsbViewerThreadStack), 10).GetValue());
        ams::os::StartThread(&g_UsbViewerThread);

        return ResultSuccess;
    }

    void Initialize() {
        UL_ASSERT(appletLoadAndApplyIdlePolicySettings());
        UpdateOperationMode();
        
        UL_ASSERT(db::Mount());

        // Remove lastt present error report
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

        g_Config = cfg::EnsureConfig();
        am::LibraryAppletSetMenuAppletId(am::LibraryAppletGetAppletIdForProgramId(g_Config.menu_program_id));

        if(g_Config.viewer_usb_enabled) {
            ams::sm::DoWithSession([]() {
                UL_ASSERT(usbCommsInitialize());
                UL_ASSERT(capsscInitialize());
            });

            PrepareUsbViewer();
            UL_ASSERT(LaunchUsbViewerThread());
        }

        UL_ASSERT(ipc::Initialize());
    }

    void Exit() {
        if(g_Config.viewer_usb_enabled) {
            usbCommsExit();
            if(g_UsbViewerMode == UsbMode::JPEG) {
                capsscExit();
            }
            operator delete[](g_UsbViewerBuffer, std::align_val_t(0x1000));
        }

        nsExit();
        pminfoExit();
        ldrShellExit();
        pmshellExit();
        db::Unmount();
    }

}

// uDaemon handles basic qlaunch functionality and serves as a back-end for uLaunch, communicating with uMenu front-end when neccessary.

int main() {
    Initialize();

    // Cache everything on startup
    cfg::CacheEverything();

    auto status = CreateStatus();
    UL_ASSERT(LaunchMenu(dmi::MenuStartMode::StartupScreen, status));

    // Loop forever - qlaunch should NEVER terminate (AM will crash otherwise)
    while(true) {
        MainLoop();
    }

    Exit();
    return 0;
}