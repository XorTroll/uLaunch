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
#include <am/am_QCommunications.hpp>
#include <util/util_Convert.hpp>
#include <cfg/cfg_Config.hpp>

extern "C"
{
    u32 __nx_applet_type = AppletType_SystemApplet;
    #ifdef Q_DEV
        size_t __nx_heap_size = 0x3000000; // Dev 48MB (still lower than official qlaunch) for debug console
    #else
        size_t __nx_heap_size = 0x800000; // 8MB - while official qlaunch uses 56MB! That's 48 extra MB for other applets
    #endif
}

namespace ams // libstratosphere needs these
{
    ncm::ProgramId CurrentProgramId = ncm::ProgramId::AppletQlaunch;
    namespace result
    {
        bool CallFatalOnResultAssertion = true;
    }
}

AccountUid selected_uid = {};
hb::TargetInput hblaunch_flag = {};
hb::TargetInput hbapplaunch_copy = {};
hb::TargetInput hbapplaunch_flag = {};
u64 titlelaunch_flag = 0;
WebCommonConfig webapplet_flag = {};
bool album_flag = false;

ams::os::Mutex latestqlock;
am::QMenuMessage latestqmenumsg = am::QMenuMessage::Invalid;

am::QDaemonStatus CreateStatus()
{
    am::QDaemonStatus status = {};
    memcpy(&status.selected_user, &selected_uid, sizeof(selected_uid));

    cfg::TitleType tmptype = cfg::TitleType::Invalid;
    if(am::ApplicationIsActive())
    {
        tmptype = cfg::TitleType::Installed;
        if(os::IsFlogTitle(am::ApplicationGetId())) tmptype = cfg::TitleType::Homebrew;
    }

    if(tmptype == cfg::TitleType::Installed) status.app_id = am::ApplicationGetId();
    else if(tmptype == cfg::TitleType::Homebrew) memcpy(&status.input, &hbapplaunch_copy, sizeof(hbapplaunch_copy));

    return status;
}

void HandleSleep()
{
    appletStartSleepSequence(true);
}

void HandleHomeButton()
{
    bool used_to_reopen_menu = false;
    if(am::LibraryAppletIsActive() && !am::LibraryAppletIsQMenu())
    {
        am::LibraryAppletTerminate();
        auto status = CreateStatus();
        am::QDaemon_LaunchQMenu(am::QMenuStartMode::Menu, status);
        return;
    }
    if(am::ApplicationIsActive())
    {
        if(am::ApplicationHasForeground())
        {
            am::HomeMenuSetForeground();
            auto status = CreateStatus();
            am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuApplicationSuspended, status);
            used_to_reopen_menu = true;
        }
    }
    if(am::LibraryAppletIsQMenu() && !used_to_reopen_menu)
    {
        std::scoped_lock _lock(latestqlock);
        latestqmenumsg = am::QMenuMessage::HomeRequest;
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
    svcSleepThread(100'000'000L);
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

void HandleQMenuMessage()
{
    if(am::LibraryAppletIsQMenu())
    {
        am::QDaemonCommandReader reader;
        if(reader)
        {
            switch(reader.GetMessage())
            {
                case am::QDaemonMessage::SetSelectedUser:
                {
                    selected_uid = reader.Read<AccountUid>();
                    reader.FinishRead();

                    break;
                }
                case am::QDaemonMessage::LaunchApplication:
                {
                    auto app_id = reader.Read<u64>();
                    reader.FinishRead();

                    if(am::ApplicationIsActive())
                    {
                        am::QDaemonCommandResultWriter res(RES_VALUE(QDaemon, ApplicationActive));
                        res.FinishWrite();
                    }
                    else if(!accountUidIsValid(&selected_uid))
                    {
                        am::QDaemonCommandResultWriter res(RES_VALUE(QDaemon, InvalidSelectedUser));
                        res.FinishWrite();
                    }
                    else if(titlelaunch_flag > 0)
                    {
                        am::QDaemonCommandResultWriter res(RES_VALUE(QDaemon, AlreadyQueued));
                        res.FinishWrite();
                    }
                    else
                    {
                        titlelaunch_flag = app_id;
                        am::QDaemonCommandResultWriter res(0);
                        res.FinishWrite();
                    }
                    break;
                }
                case am::QDaemonMessage::ResumeApplication:
                {
                    reader.FinishRead();

                    if(!am::ApplicationIsActive())
                    {
                        am::QDaemonCommandResultWriter res(RES_VALUE(QDaemon, ApplicationNotActive));
                        res.FinishWrite();
                    }
                    else
                    {
                        am::ApplicationSetForeground();
                        am::QDaemonCommandResultWriter res(0);
                        res.FinishWrite();
                    }
                    break;
                }
                case am::QDaemonMessage::TerminateApplication:
                {
                    reader.FinishRead();

                    am::ApplicationTerminate();

                    break;
                }
                case am::QDaemonMessage::LaunchHomebrewLibApplet:
                {
                    hblaunch_flag = reader.Read<hb::TargetInput>();
                    reader.FinishRead();

                    break;
                }
                case am::QDaemonMessage::LaunchHomebrewApplication:
                {
                    auto ipt = reader.Read<hb::TargetInput>();
                    reader.FinishRead();

                    if(am::ApplicationIsActive())
                    {
                        am::QDaemonCommandResultWriter res(RES_VALUE(QDaemon, ApplicationActive));
                        res.FinishWrite();
                    }
                    else if(!accountUidIsValid(&selected_uid))
                    {
                        am::QDaemonCommandResultWriter res(RES_VALUE(QDaemon, InvalidSelectedUser));
                        res.FinishWrite();
                    }
                    else if(titlelaunch_flag > 0)
                    {
                        am::QDaemonCommandResultWriter res(RES_VALUE(QDaemon, AlreadyQueued));
                        res.FinishWrite();
                    }
                    else
                    {
                        memcpy(&hbapplaunch_copy, &ipt, sizeof(ipt));
                        memcpy(&hbapplaunch_flag, &ipt, sizeof(ipt));
                        am::QDaemonCommandResultWriter res(0);
                        res.FinishWrite();
                    }
                    break;
                }
                case am::QDaemonMessage::OpenWebPage:
                {
                    webapplet_flag = reader.Read<WebCommonConfig>();
                    reader.FinishRead();

                    break;
                }
                case am::QDaemonMessage::GetSelectedUser:
                {
                    reader.FinishRead();

                    am::QDaemonCommandResultWriter res(0);
                    res.Write<AccountUid>(selected_uid);
                    res.FinishWrite();
                    
                    break;
                }
                case am::QDaemonMessage::UserHasPassword:
                {
                    auto uid = reader.Read<AccountUid>();
                    reader.FinishRead();

                    auto [rc, pass] = db::AccessPassword(uid);
                    
                    am::QDaemonCommandResultWriter res(rc);
                    res.FinishWrite();

                    break;
                }
                case am::QDaemonMessage::TryLogUser:
                {
                    auto pass = reader.Read<db::PassBlock>();
                    reader.FinishRead();

                    auto rc = db::TryLogUser(pass);

                    am::QDaemonCommandResultWriter res(rc);
                    res.FinishWrite();

                    break;
                }
                case am::QDaemonMessage::RegisterUserPassword:
                {
                    auto pass = reader.Read<db::PassBlock>();
                    reader.FinishRead();

                    auto rc = db::RegisterUserPassword(pass);

                    am::QDaemonCommandResultWriter res(rc);
                    res.FinishWrite();
                    break;
                }
                case am::QDaemonMessage::ChangeUserPassword:
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

                    am::QDaemonCommandResultWriter res(rc);
                    res.FinishWrite();

                    break;
                }
                case am::QDaemonMessage::RemoveUserPassword:
                {
                    auto pass = reader.Read<db::PassBlock>();
                    reader.FinishRead();

                    auto rc = db::TryLogUser(pass);
                    if(R_SUCCEEDED(rc))
                    {
                        rc = db::RemoveUserPassword(pass.uid);
                    }

                    am::QDaemonCommandResultWriter res(rc);
                    res.FinishWrite();

                    break;
                }
                case am::QDaemonMessage::OpenAlbum:
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
        static const size_t PointerBufferSize = 0;
        static const size_t MaxDomains = 1;
        static const size_t MaxDomainObjects = 1;
    };

    constexpr size_t MaxServers = 2;
    constexpr size_t MaxSessions = 2;

    constexpr ams::sm::ServiceName PrivateServiceName = ams::sm::ServiceName::Encode(AM_QDAEMON_SERVICE_NAME);
    ams::sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> private_service_manager;
}

namespace qdaemon
{
    void DaemonServiceMain(void *arg)
    {
        // TODO: another IPC service for other processes?

        private_service_manager.RegisterServer<ipc::IDaemonService>(PrivateServiceName, MaxSessions);

        private_service_manager.LoopProcess();
    }

    void ForegroundMain(void *arg)
    {
        u8 *usbbuf = new (std::align_val_t(0x1000)) u8[RawRGBAScreenBufferSize]();
        usbCommsInitialize();

        while(true)
        {
            bool flag;
            appletGetLastForegroundCaptureImageEx(usbbuf, RawRGBAScreenBufferSize, &flag);
            appletUpdateLastForegroundCaptureImage();
            usbCommsWrite(usbbuf, RawRGBAScreenBufferSize);
        }
        
        usbCommsExit();
        delete[] usbbuf;
    }

    Result LaunchDaemonServiceThread()
    {
        Thread daemon;
        R_TRY(threadCreate(&daemon, &DaemonServiceMain, NULL, NULL, 0x4000, 0x2b, -2));
        R_TRY(threadStart(&daemon));
        return 0;
    }

    Result LaunchForegroundThread()
    {
        Thread fg;
        R_TRY(threadCreate(&fg, &ForegroundMain, NULL, NULL, 0x4000, 0x2b, -2));
        R_TRY(threadStart(&fg));
        return 0;
    }

    void Initialize()
    {
        ams::hos::SetVersionForLibnx();
        
        Q_R_TRY(nsInitialize())
        Q_R_TRY(appletLoadAndApplyIdlePolicySettings())
        
        Q_R_TRY(db::Mount())

        fs::CreateDirectory(Q_BASE_DB_DIR);
        db::Commit();
        fs::CreateDirectory(Q_BASE_DB_DIR "/user");
        db::Commit();

        fs::CreateDirectory(Q_BASE_SD_DIR);
        fs::CreateDirectory(Q_ENTRIES_PATH);
        fs::CreateDirectory(Q_THEMES_PATH);
        fs::CreateDirectory(Q_BASE_SD_DIR "/title");
        fs::CreateDirectory(Q_BASE_SD_DIR "/user");
        fs::CreateDirectory(Q_BASE_SD_DIR "/nro");
        fs::CreateDirectory(Q_BASE_SD_DIR "/lang");

        Q_R_TRY(LaunchDaemonServiceThread())

        #ifdef Q_DEV
            // Debug testing mode
            consoleInit(NULL);
            CONSOLE_FMT("Welcome to QDaemon -> debug mode menu")
            CONSOLE_FMT("")
            CONSOLE_FMT("(A) -> Dump system save data to sd:/ulaunch/save_dump")
            CONSOLE_FMT("(B) -> Delete uLaunch's saved content in save data (official HOME menu's content won't be touched)")
            CONSOLE_FMT("(X) -> Reboot system")
            CONSOLE_FMT("(Y) -> Continue to QMenu (proceed launch)")
            CONSOLE_FMT("")

            while(true)
            {
                hidScanInput();
                auto k = hidKeysDown(CONTROLLER_P1_AUTO);
                if(k & KEY_A)
                {
                    db::Mount();
                    fs::CopyDirectory(Q_DB_MOUNT_PATH, Q_BASE_SD_DIR "/save_dump");
                    db::Unmount();
                    CONSOLE_FMT(" - Dump done.")
                }
                else if(k & KEY_B)
                {
                    db::Mount();
                    fs::DeleteDirectory(Q_BASE_DB_DIR);
                    db::Commit();
                    fs::CreateDirectory(Q_BASE_DB_DIR);
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
                    svcSleepThread(500'000'000);
                    break;
                }
                svcSleepThread(10'000'000);
            }
        #endif
    }

    void Exit()
    {
        nsExit();
        db::Unmount();
    }
}

// QDaemon handles basic qlaunch functionality and serves as a back-end for uLaunch, communicating with QMenu front-end when neccessary.
int main()
{
    qdaemon::Initialize();

    auto config = cfg::EnsureConfig();
    if(config.viewer_usb_enabled) qdaemon::LaunchForegroundThread();

    auto status = CreateStatus();
    Q_R_TRY(am::QDaemon_LaunchQMenu(am::QMenuStartMode::StartupScreen, status))

    while(true)
    {
        HandleGeneralChannel();
        HandleAppletMessage();
        HandleQMenuMessage();

        bool sth_done = false;
        if(webapplet_flag.version > 0) // A valid version in this config is always >= 0x20000
        {
            if(!am::LibraryAppletIsActive())
            {
                am::WebAppletStart(&webapplet_flag);

                sth_done = true;
                memset(&webapplet_flag, 0, sizeof(webapplet_flag));
            }
        }
        if(album_flag)
        {
            if(!am::LibraryAppletIsActive())
            {
                u8 albumflag = 2;
                am::LibraryAppletStart(AppletId_photoViewer, 0x10000, &albumflag, sizeof(albumflag));

                sth_done = true;
                album_flag = false;
            }
        }
        if(titlelaunch_flag > 0)
        {
            if(!am::LibraryAppletIsActive())
            {
                am::ApplicationStart(titlelaunch_flag, false, selected_uid);
                sth_done = true;
                titlelaunch_flag = 0;
            }
        }
        if(strlen(hbapplaunch_flag.nro_path))
        {
            if(!am::LibraryAppletIsActive())
            {
                am::ApplicationStart(OS_FLOG_APP_ID, true, selected_uid, &hbapplaunch_flag, sizeof(hbapplaunch_flag));
                sth_done = true;
                hbapplaunch_flag.nro_path[0] = '\0';
            }
        }
        if(strlen(hblaunch_flag.nro_path))
        {
            if(!am::LibraryAppletIsActive())
            {
                am::QDaemon_LaunchQHbTarget(hblaunch_flag);
                sth_done = true;
                hblaunch_flag.nro_path[0] = '\0';
            }
        }
        if(!am::LibraryAppletIsActive())
        {
            switch(am::LibraryAppletGetId())
            {
                case am::QHbTargetAppletId:
                case AppletId_web:
                case AppletId_photoViewer:
                {
                    auto status = CreateStatus();
                    am::QDaemon_LaunchQMenu(am::QMenuStartMode::Menu, status);
                    sth_done = true;
                    break;
                }
                default:
                    break;
            }
        }
        if(!sth_done)
        {
            // If nothing was done, but nothing is active... An application or applet might have crashed, terminated, failed to launch...
            // No matter what is it, we reopen QMenu in launch-error mode.
            if(!am::ApplicationIsActive() && !am::LibraryAppletIsActive())
            {
                auto status = CreateStatus();
                am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuLaunchFailure, status);
            }
        }
        svcSleepThread(10'000'000);
    }

    qdaemon::Exit();

    return 0;
}