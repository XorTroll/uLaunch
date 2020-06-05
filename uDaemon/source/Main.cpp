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

enum class USBMode : u32 {
    Invalid,
    RawRGBA,
    JPEG
};

AccountUid selected_uid = {};
hb::HbTargetParams hblaunch_flag = {};
hb::HbTargetParams hbapplaunch_flag = {};
hb::HbTargetParams hbapplaunch_flag_temp = {};
u64 titlelaunch_flag = 0;
WebCommonConfig webapplet_flag = {};
bool album_flag = false;
bool menu_restart_flag = false;
bool app_opened_hb = false;
AppletOperationMode console_mode;
u8 *usb_buf = nullptr;
u8 *usb_buf_read = nullptr;
cfg::Config config = {};
Thread ipc_thr;
Thread usb_thr;
USBMode usb_mode = USBMode::Invalid;

static constexpr size_t USBPacketSize = RawRGBAScreenBufferSize + sizeof(u32);

ams::os::Mutex g_last_menu_msg_lock(false);
dmi::MenuMessage g_last_menu_msg = dmi::MenuMessage::Invalid;

dmi::DaemonStatus CreateStatus() {
    dmi::DaemonStatus status = {};
    status.selected_user = selected_uid;

    if(am::ApplicationIsActive()) {
        if(app_opened_hb) {
            // Homebrew
            status.params = hbapplaunch_flag_temp;
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
    R_TRY(ecs::RegisterLaunchAsApplet(config.menu_program_id, (u32)stmode, "/ulaunch/bin/uMenu/", &status, sizeof(status)));
    return ResultSuccess;
}

void HandleHomeButton() {
    bool used_to_reopen_menu = false;
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
            used_to_reopen_menu = true;
        }
    }
    if(am::LibraryAppletIsMenu() && !used_to_reopen_menu) {
        std::scoped_lock lk(g_last_menu_msg_lock);
        g_last_menu_msg = dmi::MenuMessage::HomeRequest;
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
        auto msg = static_cast<os::GeneralChannelMessage>(sams.general_channel_message);
        switch(msg) {
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
    console_mode = static_cast<AppletOperationMode>(tmp_mode);
    return ResultSuccess;
}

Result HandleAppletMessage()
{
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

void HandleMenuMessage()
{
    if(am::LibraryAppletIsMenu()) {
        dmi::DaemonMessageReader reader;
        if(reader) {
            switch(reader.GetValue()) {
                case dmi::DaemonMessage::SetSelectedUser: {
                    selected_uid = reader.Read<AccountUid>();
                    reader.FinishRead();

                    break;
                }
                case dmi::DaemonMessage::LaunchApplication: {
                    auto app_id = reader.Read<u64>();
                    reader.FinishRead();

                    if(am::ApplicationIsActive()) {
                        dmi::DaemonResultWriter rc(RES_VALUE(Daemon, ApplicationActive));
                    }
                    else if(!accountUidIsValid(&selected_uid)) {
                        dmi::DaemonResultWriter rc(RES_VALUE(Daemon, InvalidSelectedUser));
                    }
                    else if(titlelaunch_flag > 0) {
                        dmi::DaemonResultWriter rc(RES_VALUE(Daemon, AlreadyQueued));
                    }
                    else {
                        titlelaunch_flag = app_id;
                        dmi::DaemonResultWriter rc(ResultSuccess);
                    }
                    break;
                }
                case dmi::DaemonMessage::ResumeApplication: {
                    reader.FinishRead();

                    if(!am::ApplicationIsActive()) {
                        dmi::DaemonResultWriter rc(RES_VALUE(Daemon, ApplicationNotActive));
                    }
                    else {
                        am::ApplicationSetForeground();
                        dmi::DaemonResultWriter rc(ResultSuccess);
                    }
                    break;
                }
                case dmi::DaemonMessage::TerminateApplication: {
                    reader.FinishRead();

                    am::ApplicationTerminate();
                    app_opened_hb = false;
                    break;
                }
                case dmi::DaemonMessage::LaunchHomebrewLibraryApplet: {
                    hblaunch_flag = reader.Read<hb::HbTargetParams>();
                    reader.FinishRead();

                    break;
                }
                case dmi::DaemonMessage::LaunchHomebrewApplication: {
                    auto app_id = reader.Read<u64>();
                    auto ipt = reader.Read<hb::HbTargetParams>();
                    reader.FinishRead();

                    if(am::ApplicationIsActive()) {
                        dmi::DaemonResultWriter rc(RES_VALUE(Daemon, ApplicationActive));
                    }
                    else if(!accountUidIsValid(&selected_uid)) {
                        dmi::DaemonResultWriter rc(RES_VALUE(Daemon, InvalidSelectedUser));
                    }
                    else if(titlelaunch_flag > 0) {
                        dmi::DaemonResultWriter rc(RES_VALUE(Daemon, AlreadyQueued));
                    }
                    else {
                        hbapplaunch_flag = ipt;
                        hbapplaunch_flag_temp = ipt;
                        titlelaunch_flag = app_id;
                        dmi::DaemonResultWriter rc(ResultSuccess);
                    }
                    break;
                }
                case dmi::DaemonMessage::OpenWebPage: {
                    webapplet_flag = reader.Read<WebCommonConfig>();
                    reader.FinishRead();

                    break;
                }
                case dmi::DaemonMessage::GetSelectedUser: {
                    reader.FinishRead();

                    dmi::DaemonResultWriter rc(ResultSuccess);
                    rc.Write<AccountUid>(selected_uid);
                    break;
                }
                case dmi::DaemonMessage::OpenAlbum:{
                    reader.FinishRead();

                    album_flag = true;
                    break;
                }
                case dmi::DaemonMessage::RestartMenu: {
                    reader.FinishRead();

                    menu_restart_flag = true;
                    break;
                }
                default:
                    break;
            }
        }
    }
}

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

    
    ams::sf::hipc::ServerManager<NumServers, ServerOptions, MaxSessions> daemon_ipc_manager;

}

static inline Result capsscCommand1204(void *buf, size_t buf_size, u64 *out_jpeg_size) {
    struct {
        u32 a;
        u64 b;
    } in = {0, 10000000000};
    return serviceDispatchInOut(capsscGetServiceSession(), 1204, in, *out_jpeg_size,
        .buffer_attrs = { SfBufferAttr_HipcMapTransferAllowsNonSecure | SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buf, buf_size } },
    );
}

namespace impl {

    void IPCManagerThread(void *arg) {
        UL_ASSERT(daemon_ipc_manager.RegisterServer<ipc::IPrivateService>(PrivateServiceName, MaxPrivateSessions).GetValue());
        UL_ASSERT(daemon_ipc_manager.RegisterServer<ipc::IPublicService>(PublicServiceName, MaxPublicSessions).GetValue());
        daemon_ipc_manager.LoopProcess();
    }

    void USBViewerRGBAThread(void *arg) {
        while(true) {
            bool tmp_flag;
            appletGetLastForegroundCaptureImageEx(usb_buf_read, RawRGBAScreenBufferSize, &tmp_flag);
            appletUpdateLastForegroundCaptureImage();
            usbCommsWrite(usb_buf, USBPacketSize);
            svcSleepThread(10'000'000l);
        }
    }

    void USBViewerJPEGThread(void *arg) {
        while(true) {
            u64 tmp_size;
            capsscCommand1204(usb_buf_read, RawRGBAScreenBufferSize, &tmp_size);
            usbCommsWrite(usb_buf, USBPacketSize);
            svcSleepThread(10'000'000l);
        }
    }

    void PrepareUSBViewer() {
        usb_buf = new (std::align_val_t(0x1000), std::nothrow) u8[USBPacketSize]();
        usb_buf_read = usb_buf + sizeof(u32);
        u64 tmp_size;
        if(R_SUCCEEDED(capsscCommand1204(usb_buf_read, RawRGBAScreenBufferSize, &tmp_size))) {
            usb_mode = USBMode::JPEG;
        }
        else {
            usb_mode = USBMode::RawRGBA;
            capsscExit();
        }
        *reinterpret_cast<u32*>(usb_buf) = static_cast<u32>(usb_mode);
    }

    void LoopUpdate() {
        HandleGeneralChannel();
        HandleAppletMessage();
        HandleMenuMessage();

        bool sth_done = false;
        // A valid version in this config is always >= 0x20000
        if(webapplet_flag.version > 0) {
            if(!am::LibraryAppletIsActive()) {
                UL_ASSERT(am::WebAppletStart(&webapplet_flag));

                sth_done = true;
                webapplet_flag = {};
            }
        }
        if(menu_restart_flag) {
            if(!am::LibraryAppletIsActive()) {
                auto status = CreateStatus();
                UL_ASSERT(LaunchMenu(dmi::MenuStartMode::StartupScreen, status));

                sth_done = true;
                menu_restart_flag = false;
            }
        }
        if(album_flag) {
            if(!am::LibraryAppletIsActive()) {
                u8 albumflag = 2;
                UL_ASSERT(am::LibraryAppletStart(AppletId_photoViewer, 0x10000, &albumflag, sizeof(albumflag)));

                sth_done = true;
                album_flag = false;
            }
        }
        if(titlelaunch_flag > 0) {
            if(!am::LibraryAppletIsActive()) {
                if(strlen(hbapplaunch_flag.nro_path)) {
                    auto params = hb::HbTargetParams::Create(hbapplaunch_flag.nro_path, hbapplaunch_flag.nro_argv, false);
                    UL_ASSERT(ecs::RegisterLaunchAsApplication(titlelaunch_flag, "/ulaunch/bin/uHbTarget/app/", &params, sizeof(params), selected_uid));
                    sth_done = true;
                    app_opened_hb = true;
                    titlelaunch_flag = 0;
                    hbapplaunch_flag.nro_path[0] = '\0';
                }
                else {
                    UL_ASSERT(am::ApplicationStart(titlelaunch_flag, false, selected_uid));
                    sth_done = true;
                    titlelaunch_flag = 0;
                }
            }
        }
        if(strlen(hblaunch_flag.nro_path)) {
            if(!am::LibraryAppletIsActive()) {
                auto params = hb::HbTargetParams::Create(hblaunch_flag.nro_path, hblaunch_flag.nro_argv, false);
                UL_ASSERT(ecs::RegisterLaunchAsApplet(config.homebrew_applet_program_id, 0, "/ulaunch/bin/uHbTarget/applet/", &params, sizeof(params)));
                sth_done = true;
                hblaunch_flag.nro_path[0] = '\0';
            }
        }
        if(!am::LibraryAppletIsActive()) {
            auto cur_id = am::LibraryAppletGetId();
            if((cur_id == AppletId_web) || (cur_id == AppletId_photoViewer) || (cur_id == config.homebrew_applet_program_id)) {
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
        svcSleepThread(10'000'000);
    }

    Result LaunchIPCManagerThread() {
        R_TRY(threadCreate(&ipc_thr, &IPCManagerThread, nullptr, nullptr, 0x4000, 0x2b, -2));
        R_TRY(threadStart(&ipc_thr));
        return ResultSuccess;
    }

    Result LaunchUSBViewerThread() {
        switch(usb_mode) {
            case USBMode::RawRGBA: {
                R_TRY(threadCreate(&usb_thr, &USBViewerRGBAThread, nullptr, nullptr, 0x4000, 0x2b, -2));
                break;
            }
            case USBMode::JPEG: {
                R_TRY(threadCreate(&usb_thr, &USBViewerJPEGThread, nullptr, nullptr, 0x4000, 0x2b, -2));
                break;
            }
            default:
                return ResultSuccess;
        }
        R_TRY(threadStart(&usb_thr));
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

        config = cfg::EnsureConfig();
        am::LibraryAppletSetMenuAppletId(am::LibraryAppletGetAppletIdForProgramId(config.menu_program_id));

        if(config.viewer_usb_enabled) {
            UL_ASSERT(usbCommsInitialize());
            UL_ASSERT(capsscInitialize());
            PrepareUSBViewer();
            UL_ASSERT(LaunchUSBViewerThread());
        }

        UL_ASSERT(LaunchIPCManagerThread());
    }

    void Exit() {
        if(config.viewer_usb_enabled) {
            usbCommsExit();
            if(usb_mode == USBMode::JPEG) {
                capsscExit();
            }
            operator delete[](usb_buf, std::align_val_t(0x1000), std::nothrow);
        }

        nsExit();
        pminfoExit();
        ecs::Exit();
        db::Unmount();
    }

}

// uDaemon handles basic qlaunch functionality and serves as a back-end for uLaunch, communicating with uMenu front-end when neccessary.

int main() {
    impl::Initialize();

    // Cache everything on startup
    cfg::CacheEverything();

    auto status = CreateStatus();
    UL_ASSERT(LaunchMenu(dmi::MenuStartMode::StartupScreen, status));

    // Loop forever - qlaunch should NEVER terminate
    while(true) {
        impl::LoopUpdate();
    }

    impl::Exit();
    return 0;
}