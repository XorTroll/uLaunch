#include <ecs/ecs_ExternalContent.hpp>
#include <ipc/ipc_IDaemonService.hpp>
#include <db/db_Save.hpp>
#include <os/os_Titles.hpp>
#include <os/os_HomeMenu.hpp>
#include <os/os_Account.hpp>
#include <os/os_Misc.hpp>
#include <fs/fs_Stdio.hpp>
#include <am/am_Application.hpp>
#include <am/am_LibraryApplet.hpp>
#include <am/am_HomeMenu.hpp>
#include <am/am_DaemonMenuInteraction.hpp>
#include <util/util_Convert.hpp>
#include <cfg/cfg_Config.hpp>

extern "C"
{
    u32 __nx_applet_type = AppletType_SystemApplet;
    TimeServiceType __nx_time_service_type = TimeServiceType_System;
    #if UL_DEV
        size_t __nx_heap_size = 0x3000000; // Dev builds use 48MB (still lower than official qlaunch) for debug console
    #else
        size_t __nx_heap_size = 0x800000; // uDaemon uses 8MB - while official qlaunch uses 56MB! That's 48 extra MB for other applets
    #endif
}

// Needed by libstratosphere
namespace ams
{
    ncm::ProgramId CurrentProgramId = ncm::ProgramId::AppletQlaunch;
    namespace result
    {
        bool CallFatalOnResultAssertion = true;
    }
}

AccountUid selected_uid = {};
hb::HbTargetParams hblaunch_flag = {};
hb::HbTargetParams hbapplaunch_flag = {};
hb::HbTargetParams hbapplaunch_flag_temp = {};
u64 titlelaunch_flag = 0;
WebCommonConfig webapplet_flag = {};
bool album_flag = false;
bool app_opened_hb = false;
u8 *usbbuf = nullptr;
cfg::Config config = {};
Thread ipc_thr;
Thread usb_thr;

// This way the menu isn't loaded, even if nothing but the home menu is present, which outside of the debug menu is considered an invalid state
bool debug_menu = false;

ams::os::Mutex latestqlock;
am::MenuMessage latestqmenumsg = am::MenuMessage::Invalid;

am::DaemonStatus CreateStatus()
{
    am::DaemonStatus status = {};
    memcpy(&status.selected_user, &selected_uid, sizeof(selected_uid));

    cfg::TitleType tmptype = cfg::TitleType::Invalid;
    if(am::ApplicationIsActive())
    {
        tmptype = cfg::TitleType::Installed;
        if(app_opened_hb) tmptype = cfg::TitleType::Homebrew;
    }

    if(tmptype == cfg::TitleType::Installed) status.app_id = am::ApplicationGetId();
    else if(tmptype == cfg::TitleType::Homebrew) memcpy(&status.params, &hbapplaunch_flag_temp, sizeof(hbapplaunch_flag_temp));

    return status;
}

void HandleSleep()
{
    appletStartSleepSequence(true);
}

Result LaunchMenu(am::MenuStartMode stmode, am::DaemonStatus status)
{
    if(debug_menu) return 0;
    return ecs::RegisterLaunchAsApplet(config.menu_program_id, (u32)stmode, "/ulaunch/bin/uMenu/", &status, sizeof(status));
}

void HandleHomeButton()
{
    bool used_to_reopen_menu = false;
    if(am::LibraryAppletIsActive() && !am::LibraryAppletIsMenu())
    {
        am::LibraryAppletTerminate();
        auto status = CreateStatus();
        UL_R_TRY(LaunchMenu(am::MenuStartMode::Menu, status));
        return;
    }
    if(am::ApplicationIsActive())
    {
        if(am::ApplicationHasForeground())
        {
            am::HomeMenuSetForeground();
            auto status = CreateStatus();
            UL_R_TRY(LaunchMenu(am::MenuStartMode::MenuApplicationSuspended, status));
            used_to_reopen_menu = true;
        }
    }
    if(am::LibraryAppletIsMenu() && !used_to_reopen_menu)
    {
        std::scoped_lock _lock(latestqlock);
        latestqmenumsg = am::MenuMessage::HomeRequest;
    }
}

void HandleGeneralChannel()
{
    AppletStorage sams_st;
    auto rc = appletPopFromGeneralChannel(&sams_st);
    if(R_SUCCEEDED(rc))
    {
        os::SystemAppletMessage sams = {};
        rc = appletStorageRead(&sams_st, 0, &sams, sizeof(os::SystemAppletMessage));
        appletStorageClose(&sams_st);
        if(R_SUCCEEDED(rc))
        {
            if(sams.magic == os::SAMSMagic)
            {
                os::GeneralChannelMessage msg = (os::GeneralChannelMessage)sams.message;
                switch(msg)
                {
                    case os::GeneralChannelMessage::HomeButton: // Usually this doesn't happen, HOME is detected by applet messages...?
                    {
                        HandleHomeButton();
                        break;
                    }
                    case os::GeneralChannelMessage::Shutdown:
                    {
                        appletStartShutdownSequence();
                        break;
                    }
                    case os::GeneralChannelMessage::Reboot:
                    {
                        appletStartRebootSequence();
                        break;
                    }
                    case os::GeneralChannelMessage::Sleep:
                    {
                        HandleSleep();
                        break;
                    }
                    default: // We don't have anything special to do for the rest
                        break;
                }
            }
        }
    }
}

void HandleAppletMessage()
{
    u32 nmsg = 0;
    auto rc = appletGetMessage(&nmsg);
    if(R_SUCCEEDED(rc))
    {
        os::AppletMessage msg = (os::AppletMessage)nmsg;
        switch(msg)
        {
            case os::AppletMessage::HomeButton:
            {
                HandleHomeButton();
                break;
            }
            case os::AppletMessage::SdCardOut:
            {
                // SD card out? Cya!
                appletStartShutdownSequence();
                break;
            }
            case os::AppletMessage::PowerButton:
            {
                HandleSleep();
                break;
            }
            default:
                break;
        }
    }
    svcSleepThread(100'000'000L);
}

void HandleMenuMessage()
{
    if(am::LibraryAppletIsMenu())
    {
        am::DaemonCommandReader reader;
        if(reader)
        {
            switch(reader.GetMessage())
            {
                case am::DaemonMessage::SetSelectedUser:
                {
                    selected_uid = reader.Read<AccountUid>();
                    reader.FinishRead();

                    break;
                }
                case am::DaemonMessage::LaunchApplication:
                {
                    auto app_id = reader.Read<u64>();
                    reader.FinishRead();

                    if(am::ApplicationIsActive())
                    {
                        am::DaemonCommandResultWriter res(RES_VALUE(Daemon, ApplicationActive));
                        res.FinishWrite();
                    }
                    else if(!accountUidIsValid(&selected_uid))
                    {
                        am::DaemonCommandResultWriter res(RES_VALUE(Daemon, InvalidSelectedUser));
                        res.FinishWrite();
                    }
                    else if(titlelaunch_flag > 0)
                    {
                        am::DaemonCommandResultWriter res(RES_VALUE(Daemon, AlreadyQueued));
                        res.FinishWrite();
                    }
                    else
                    {
                        titlelaunch_flag = app_id;
                        am::DaemonCommandResultWriter res(0);
                        res.FinishWrite();
                    }
                    break;
                }
                case am::DaemonMessage::ResumeApplication:
                {
                    reader.FinishRead();

                    if(!am::ApplicationIsActive())
                    {
                        am::DaemonCommandResultWriter res(RES_VALUE(Daemon, ApplicationNotActive));
                        res.FinishWrite();
                    }
                    else
                    {
                        am::ApplicationSetForeground();
                        am::DaemonCommandResultWriter res(0);
                        res.FinishWrite();
                    }
                    break;
                }
                case am::DaemonMessage::TerminateApplication:
                {
                    reader.FinishRead();

                    am::ApplicationTerminate();
                    app_opened_hb = false;

                    break;
                }
                case am::DaemonMessage::LaunchHomebrewLibraryApplet:
                {
                    hblaunch_flag = reader.Read<hb::HbTargetParams>();
                    reader.FinishRead();

                    break;
                }
                case am::DaemonMessage::LaunchHomebrewApplication:
                {
                    auto app_id = reader.Read<u64>();
                    auto ipt = reader.Read<hb::HbTargetParams>();
                    reader.FinishRead();

                    if(am::ApplicationIsActive())
                    {
                        am::DaemonCommandResultWriter res(RES_VALUE(Daemon, ApplicationActive));
                        res.FinishWrite();
                    }
                    else if(!accountUidIsValid(&selected_uid))
                    {
                        am::DaemonCommandResultWriter res(RES_VALUE(Daemon, InvalidSelectedUser));
                        res.FinishWrite();
                    }
                    else if(titlelaunch_flag > 0)
                    {
                        am::DaemonCommandResultWriter res(RES_VALUE(Daemon, AlreadyQueued));
                        res.FinishWrite();
                    }
                    else
                    {
                        memcpy(&hbapplaunch_flag, &ipt, sizeof(ipt));
                        memcpy(&hbapplaunch_flag_temp, &ipt, sizeof(ipt));
                        titlelaunch_flag = app_id;
                        am::DaemonCommandResultWriter res(0);
                        res.FinishWrite();
                    }
                    break;
                }
                case am::DaemonMessage::OpenWebPage:
                {
                    webapplet_flag = reader.Read<WebCommonConfig>();
                    reader.FinishRead();

                    break;
                }
                case am::DaemonMessage::GetSelectedUser:
                {
                    reader.FinishRead();

                    am::DaemonCommandResultWriter res(0);
                    res.Write<AccountUid>(selected_uid);
                    res.FinishWrite();
                    
                    break;
                }
                case am::DaemonMessage::UserHasPassword:
                {
                    auto uid = reader.Read<AccountUid>();
                    reader.FinishRead();

                    auto [rc, pass] = db::AccessPassword(uid);
                    
                    am::DaemonCommandResultWriter res(rc);
                    res.FinishWrite();

                    break;
                }
                case am::DaemonMessage::TryLogUser:
                {
                    auto pass = reader.Read<db::PassBlock>();
                    reader.FinishRead();

                    auto rc = db::TryLogUser(pass);

                    am::DaemonCommandResultWriter res(rc);
                    res.FinishWrite();

                    break;
                }
                case am::DaemonMessage::RegisterUserPassword:
                {
                    auto pass = reader.Read<db::PassBlock>();
                    reader.FinishRead();

                    auto rc = db::RegisterUserPassword(pass);

                    am::DaemonCommandResultWriter res(rc);
                    res.FinishWrite();
                    break;
                }
                case am::DaemonMessage::ChangeUserPassword:
                {
                    auto pass = reader.Read<db::PassBlock>();
                    auto newpass = reader.Read<db::PassBlock>();
                    reader.FinishRead();

                    auto rc = db::TryLogUser(pass);
                    if(R_SUCCEEDED(rc))
                    {
                        rc = db::RemoveUserPassword(pass.uid);
                        if(R_SUCCEEDED(rc)) rc = db::RegisterUserPassword(newpass);
                    }

                    am::DaemonCommandResultWriter res(rc);
                    res.FinishWrite();

                    break;
                }
                case am::DaemonMessage::RemoveUserPassword:
                {
                    auto pass = reader.Read<db::PassBlock>();
                    reader.FinishRead();

                    auto rc = db::TryLogUser(pass);
                    if(R_SUCCEEDED(rc))
                    {
                        rc = db::RemoveUserPassword(pass.uid);
                    }

                    am::DaemonCommandResultWriter res(rc);
                    res.FinishWrite();

                    break;
                }
                case am::DaemonMessage::OpenAlbum:
                {
                    reader.FinishRead();
                    album_flag = true;

                    break;
                }
                default:
                    break;
            }
        }
    }
}

namespace
{
    struct ServerOptions
    {
        static const size_t PointerBufferSize = 0x400;
        static const size_t MaxDomains = 0x40;
        static const size_t MaxDomainObjects = 0x40;
    };

    constexpr size_t MaxServers = 2;
    constexpr size_t MaxSessions = 2;

    constexpr ams::sm::ServiceName PrivateServiceName = ams::sm::ServiceName::Encode(AM_DAEMON_PRIVATE_SERVICE_NAME);
    ams::sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> private_service_manager;
}

namespace daemn
{
    void IPCManagerThread(void *arg)
    {
        UL_R_TRY(private_service_manager.RegisterServer<ipc::IDaemonService>(PrivateServiceName, MaxSessions).GetValue());
        private_service_manager.LoopProcess();
    }

    void USBViewerThread(void *arg)
    {
        while(true)
        {
            bool flag;
            appletGetLastForegroundCaptureImageEx(usbbuf, RawRGBAScreenBufferSize, &flag);
            appletUpdateLastForegroundCaptureImage();
            usbCommsWrite(usbbuf, RawRGBAScreenBufferSize);
            svcSleepThread(10'000'000l);
        }
    }

    void LoopUpdate()
    {
        HandleGeneralChannel();
        HandleAppletMessage();
        HandleMenuMessage();

        bool sth_done = false;
        if(webapplet_flag.version > 0) // A valid version in this config is always >= 0x20000
        {
            if(!am::LibraryAppletIsActive())
            {
                UL_R_TRY(am::WebAppletStart(&webapplet_flag));

                sth_done = true;
                memset(&webapplet_flag, 0, sizeof(webapplet_flag));
            }
        }
        if(album_flag)
        {
            if(!am::LibraryAppletIsActive())
            {
                u8 albumflag = 2;
                UL_R_TRY(am::LibraryAppletStart(AppletId_photoViewer, 0x10000, &albumflag, sizeof(albumflag)));

                sth_done = true;
                album_flag = false;
            }
        }
        if(titlelaunch_flag > 0)
        {
            if(!am::LibraryAppletIsActive())
            {
                if(strlen(hbapplaunch_flag.nro_path))
                {
                    auto params = hb::HbTargetParams::Create(hbapplaunch_flag.nro_path, hbapplaunch_flag.nro_argv, false);
                    UL_R_TRY(ecs::RegisterLaunchAsApplication(titlelaunch_flag, "/ulaunch/bin/uHbTarget/app/", &params, sizeof(params), selected_uid));
                    sth_done = true;
                    app_opened_hb = true;
                    titlelaunch_flag = 0;
                    hbapplaunch_flag.nro_path[0] = '\0';
                }
                else
                {
                    UL_R_TRY(am::ApplicationStart(titlelaunch_flag, false, selected_uid));
                    sth_done = true;
                    titlelaunch_flag = 0;
                }
            }
        }
        if(strlen(hblaunch_flag.nro_path))
        {
            if(!am::LibraryAppletIsActive())
            {
                auto params = hb::HbTargetParams::Create(hblaunch_flag.nro_path, hblaunch_flag.nro_argv, false);
                UL_R_TRY(ecs::RegisterLaunchAsApplet(config.homebrew_applet_program_id, 0, "/ulaunch/bin/uHbTarget/applet/", &params, sizeof(params)));
                sth_done = true;
                hblaunch_flag.nro_path[0] = '\0';
            }
        }
        if(!am::LibraryAppletIsActive())
        {
            auto cur_id = am::LibraryAppletGetId();
            if((cur_id == AppletId_web) || (cur_id == AppletId_photoViewer) || (cur_id == config.homebrew_applet_program_id))
            {
                auto status = CreateStatus();
                UL_R_TRY(LaunchMenu(am::MenuStartMode::Menu, status));
                sth_done = true;
            }
        }
        if(!sth_done && !debug_menu)
        {
            // If nothing was done, but nothing is active... An application or applet might have crashed, terminated, failed to launch...
            // No matter what is it, we reopen Menu in launch-error mode.
            if(!am::ApplicationIsActive() && !am::LibraryAppletIsActive())
            {
                auto status = CreateStatus();
                UL_R_TRY(LaunchMenu(am::MenuStartMode::MenuLaunchFailure, status));
            }
        }
    }

    Result LaunchIPCManagerThread()
    {
        R_TRY(threadCreate(&ipc_thr, &IPCManagerThread, NULL, NULL, 0x4000, 0x2b, -2));
        R_TRY(threadStart(&ipc_thr));
        return 0;
    }

    Result LaunchUSBViewerThread()
    {
        R_TRY(threadCreate(&usb_thr, &USBViewerThread, NULL, NULL, 0x4000, 0x2b, -2));
        R_TRY(threadStart(&usb_thr));
        return 0;
    }

    void Initialize()
    {
        ams::hos::SetVersionForLibnx();
        
        UL_R_TRY(nsInitialize())
        UL_R_TRY(appletLoadAndApplyIdlePolicySettings())
        UL_R_TRY(ecs::Initialize())
        
        UL_R_TRY(db::Mount())

        fs::CreateDirectory(UL_BASE_DB_DIR);
        db::Commit();
        fs::CreateDirectory(UL_BASE_DB_DIR "/user");
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

        if(config.viewer_usb_enabled)
        {
            UL_R_TRY(usbCommsInitialize());
            usbbuf = new (std::align_val_t(0x1000)) u8[RawRGBAScreenBufferSize]();
            UL_R_TRY(LaunchUSBViewerThread());
        }

        UL_R_TRY(LaunchIPCManagerThread())

        #if UL_DEV
            // Debug testing mode
            debug_menu = true;

            consoleInit(NULL);
            CONSOLE_FMT("Welcome to uDaemon -> debug mode menu")
            CONSOLE_FMT("")
            CONSOLE_FMT("(A) -> Dump system save data to sd:/ulaunch/save_dump")
            CONSOLE_FMT("(B) -> Delete uLaunch's saved content in save data (official HOME menu's content won't be touched)")
            CONSOLE_FMT("(X) -> Reboot system")
            CONSOLE_FMT("(Y) -> Continue to uMenu (launch normally)")
            CONSOLE_FMT("")

            while(true)
            {
                hidScanInput();
                auto k = hidKeysDown(CONTROLLER_P1_AUTO);
                if(k & KEY_A)
                {
                    db::Mount();
                    fs::CopyDirectory(UL_DB_MOUNT_PATH, UL_BASE_SD_DIR "/save_dump");
                    db::Unmount();
                    CONSOLE_FMT(" - Dump done.")
                }
                else if(k & KEY_B)
                {
                    db::Mount();
                    fs::DeleteDirectory(UL_BASE_DB_DIR);
                    db::Commit();
                    fs::CreateDirectory(UL_BASE_DB_DIR);
                    db::Commit();
                    db::Unmount();
                    CONSOLE_FMT(" - Cleanup done.")
                }
                else if(k & KEY_X)
                {
                    CONSOLE_FMT(" - Rebooting...")
                    svcSleepThread(200'000'000);
                    appletStartRebootSequence();
                }
                else if(k & KEY_Y)
                {
                    CONSOLE_FMT(" - Proceeding with launch...")
                    svcSleepThread(100'000'000);
                    break;
                }
                LoopUpdate();
                svcSleepThread(10'000'000);
            }
            debug_menu = false;
        #endif
    }

    void Exit()
    {
        if(config.viewer_usb_enabled)
        {
            usbCommsExit();
            operator delete[](usbbuf, std::align_val_t(0x1000));
        }

        nsExit();
        ecs::Exit();
        db::Unmount();
    }
}

// Daemon handles basic qlaunch functionality and serves as a back-end for uLaunch, communicating with Menu front-end when neccessary.
int main()
{
    daemn::Initialize();

    // Cache everything on startup
    cfg::CacheEverything();

    auto status = CreateStatus();
    UL_R_TRY(LaunchMenu(am::MenuStartMode::StartupScreen, status))

    while(true)
    {
        daemn::LoopUpdate();
        svcSleepThread(10'000'000);
    }

    daemn::Exit();

    return 0;
}