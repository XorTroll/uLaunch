#include <ecs/ecs_ExternalContent.hpp>
#include <ipc/ipc_IPrivateService.hpp>
#include <ipc/ipc_IPublicService.hpp>
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

    u32 __nx_applet_type = AppletType_SystemApplet;
    TimeServiceType __nx_time_service_type = TimeServiceType_System;

    // uDaemon uses 8MB - while official qlaunch uses 56MB! That's 48 extra MB for other applets!
    size_t __nx_heap_size = 0x800000;

}

// Needed by libstratosphere

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemAppletId::Qlaunch;
    
    namespace result {

        bool CallFatalOnResultAssertion = true;

    }

}

ams::os::Mutex g_LastMenuMessageLock(false);
dmi::MenuMessage g_LastMenuMessage = dmi::MenuMessage::Invalid;

namespace {

    struct ServerOptions {
        static const size_t PointerBufferSize = 0x400;
        static const size_t MaxDomains = 0x40;
        static const size_t MaxDomainObjects = 0x40;
    };

    constexpr size_t MaxPrivateSessions = 1;
    constexpr ams::sm::ServiceName PrivateServiceName = ams::sm::ServiceName::Encode(AM_DAEMON_PRIVATE_SERVICE_NAME);

    constexpr size_t MaxPublicSessions = 0x20;
    constexpr ams::sm::ServiceName PublicServiceName = ams::sm::ServiceName::Encode(AM_DAEMON_PUBLIC_SERVICE_NAME);

    constexpr size_t NumServers = 2;
    constexpr size_t MaxSessions = MaxPrivateSessions + MaxPublicSessions + 1;

    
    ams::sf::hipc::ServerManager<NumServers, ServerOptions, MaxSessions> g_IpcManager;

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
    Thread g_IpcServerThread;
    Thread g_UsbViewerThread;
    UsbMode g_UsbViewerMode = UsbMode::Invalid;

    // In the USB packet, the first u32 / the first 4 bytes are the USB mode (raw RGBA or JPEG, depending on what the console supports)

    constexpr size_t UsbPacketSize = RawRGBAScreenBufferSize + sizeof(u32);

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

    Result LaunchMenu(dmi::MenuStartMode stmode, dmi::DaemonStatus status) {
        R_TRY(ecs::RegisterLaunchAsApplet(g_Config.menu_program_id, static_cast<u32>(stmode), "/ulaunch/bin/uMenu/", &status, sizeof(status)));
        return ResultSuccess;
    }

    void HandleHomeButton() {
        if(am::LibraryAppletIsActive() && !am::LibraryAppletIsMenu()) {
            am::LibraryAppletTerminate();
            auto status = CreateStatus();
            UL_ASSERT(LaunchMenu(dmi::MenuStartMode::Menu, status));
            return;
        }
        if(am::ApplicationIsActive()) {
            if(am::ApplicationHasForeground()) {
                am::HomeMenuSetForeground();
                auto status = CreateStatus();
                UL_ASSERT(LaunchMenu(dmi::MenuStartMode::MenuApplicationSuspended, status));
                return;
            }
        }
        if(am::LibraryAppletIsMenu()) {
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
            dmi::DaemonMessageReader reader;
            if(reader) {
                switch(reader.GetValue()) {
                    case dmi::DaemonMessage::SetSelectedUser: {
                        g_SelectedUser = reader.Read<AccountUid>();
                        reader.FinishRead();

                        break;
                    }
                    case dmi::DaemonMessage::LaunchApplication: {
                        auto app_id = reader.Read<u64>();
                        reader.FinishRead();

                        if(am::ApplicationIsActive()) {
                            dmi::DaemonResultWriter rc(RES_VALUE(Daemon, ApplicationActive));
                            rc.FinishWrite();
                        }
                        else if(!accountUidIsValid(&g_SelectedUser)) {
                            dmi::DaemonResultWriter rc(RES_VALUE(Daemon, InvalidSelectedUser));
                            rc.FinishWrite();
                        }
                        else if(g_ApplicationLaunchFlag > 0) {
                            dmi::DaemonResultWriter rc(RES_VALUE(Daemon, AlreadyQueued));
                            rc.FinishWrite();
                        }
                        else {
                            g_ApplicationLaunchFlag = app_id;
                            dmi::DaemonResultWriter rc(ResultSuccess);
                            rc.FinishWrite();
                        }
                        break;
                    }
                    case dmi::DaemonMessage::ResumeApplication: {
                        reader.FinishRead();

                        if(!am::ApplicationIsActive()) {
                            dmi::DaemonResultWriter rc(RES_VALUE(Daemon, ApplicationNotActive));
                            rc.FinishWrite();
                        }
                        else {
                            am::ApplicationSetForeground();
                            dmi::DaemonResultWriter rc(ResultSuccess);
                            rc.FinishWrite();
                        }
                        break;
                    }
                    case dmi::DaemonMessage::TerminateApplication: {
                        reader.FinishRead();

                        am::ApplicationTerminate();
                        g_HbTargetOpenedAsApplication = false;
                        break;
                    }
                    case dmi::DaemonMessage::LaunchHomebrewLibraryApplet: {
                        g_HbTargetLaunchFlag = reader.Read<hb::HbTargetParams>();
                        reader.FinishRead();

                        break;
                    }
                    case dmi::DaemonMessage::LaunchHomebrewApplication: {
                        auto app_id = reader.Read<u64>();
                        auto ipt = reader.Read<hb::HbTargetParams>();
                        reader.FinishRead();

                        if(am::ApplicationIsActive()) {
                            dmi::DaemonResultWriter rc(RES_VALUE(Daemon, ApplicationActive));
                            rc.FinishWrite();
                        }
                        else if(!accountUidIsValid(&g_SelectedUser)) {
                            dmi::DaemonResultWriter rc(RES_VALUE(Daemon, InvalidSelectedUser));
                            rc.FinishWrite();
                        }
                        else if(g_ApplicationLaunchFlag > 0) {
                            dmi::DaemonResultWriter rc(RES_VALUE(Daemon, AlreadyQueued));
                            rc.FinishWrite();
                        }
                        else {
                            g_HbTargetApplicationLaunchFlag = ipt;
                            g_HbTargetApplicationLaunchFlagCopy = ipt;
                            g_ApplicationLaunchFlag = app_id;
                            dmi::DaemonResultWriter rc(ResultSuccess);
                            rc.FinishWrite();
                        }
                        break;
                    }
                    case dmi::DaemonMessage::OpenWebPage: {
                        g_WebAppletLaunchFlag = reader.Read<WebCommonConfig>();
                        reader.FinishRead();

                        break;
                    }
                    case dmi::DaemonMessage::GetSelectedUser: {
                        reader.FinishRead();

                        dmi::DaemonResultWriter rc(ResultSuccess);
                        rc.Write<AccountUid>(g_SelectedUser);
                        rc.FinishWrite();
                        break;
                    }
                    case dmi::DaemonMessage::OpenAlbum: {
                        reader.FinishRead();

                        g_AlbumAppletLaunchFlag = true;
                        break;
                    }
                    case dmi::DaemonMessage::RestartMenu: {
                        reader.FinishRead();

                        g_MenuRestartFlag = true;
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

    void IpcServerThread(void *arg) {
        UL_ASSERT(g_IpcManager.RegisterServer<ipc::IPrivateService>(PrivateServiceName, MaxPrivateSessions).GetValue());
        UL_ASSERT(g_IpcManager.RegisterServer<ipc::IPublicService>(PublicServiceName, MaxPublicSessions).GetValue());
        g_IpcManager.LoopProcess();
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
        g_UsbViewerReadBuffer = g_UsbViewerBuffer + sizeof(u32);
        u64 tmp_size;
        if(R_SUCCEEDED(capsscCaptureJpegScreenShot(&tmp_size, g_UsbViewerReadBuffer, RawRGBAScreenBufferSize, ViLayerStack_Default, UINT64_MAX))) {
            g_UsbViewerMode = UsbMode::JPEG;
        }
        else {
            g_UsbViewerMode = UsbMode::RawRGBA;
            capsscExit();
        }
        *reinterpret_cast<u32*>(g_UsbViewerBuffer) = static_cast<u32>(g_UsbViewerMode);
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
                    UL_ASSERT(ecs::RegisterLaunchAsApplication(g_ApplicationLaunchFlag, "/ulaunch/bin/uHbTarget/app/", &params, sizeof(params), g_SelectedUser));
                    
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
                UL_ASSERT(ecs::RegisterLaunchAsApplet(g_Config.homebrew_applet_program_id, 0, "/ulaunch/bin/uHbTarget/applet/", &params, sizeof(params)));
                
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

    Result LaunchIpcServerThread() {
        R_TRY(threadCreate(&g_IpcServerThread, &IpcServerThread, nullptr, nullptr, 0x4000, 0x2B, -2));
        R_TRY(threadStart(&g_IpcServerThread));
        
        return ResultSuccess;
    }

    Result LaunchUsbViewerThread() {
        switch(g_UsbViewerMode) {
            case UsbMode::RawRGBA: {
                R_TRY(threadCreate(&g_UsbViewerThread, &UsbViewerRGBAThread, nullptr, nullptr, 0x4000, 0x2B, -2));
                break;
            }
            case UsbMode::JPEG: {
                R_TRY(threadCreate(&g_UsbViewerThread, &UsbViewerJPEGThread, nullptr, nullptr, 0x4000, 0x2B, -2));
                break;
            }
            default:
                return ResultSuccess;
        }
        R_TRY(threadStart(&g_UsbViewerThread));

        return ResultSuccess;
    }

    void Initialize() {
        UL_ASSERT(setsysInitialize());
        SetSysFirmwareVersion fwver = {};
        UL_ASSERT(setsysGetFirmwareVersion(&fwver));
        hosversionSet(MAKEHOSVERSION(fwver.major, fwver.minor, fwver.micro));
        setsysExit();

        UL_ASSERT(nsInitialize());
        UL_ASSERT(pminfoInitialize());
        UL_ASSERT(appletLoadAndApplyIdlePolicySettings());
        UpdateOperationMode();
        UL_ASSERT(ecs::Initialize());
        
        UL_ASSERT(db::Mount());

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
            UL_ASSERT(usbCommsInitialize());
            UL_ASSERT(capsscInitialize());

            PrepareUsbViewer();
            UL_ASSERT(LaunchUsbViewerThread());
        }

        UL_ASSERT(LaunchIpcServerThread());
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
        ecs::Exit();
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