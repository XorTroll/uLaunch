#include <db/db_Save.hpp>
#include <os/os_Titles.hpp>
#include <os/os_HomeMenu.hpp>
#include <os/os_Account.hpp>
#include <fs/fs_Stdio.hpp>
#include <am/am_Application.hpp>
#include <am/am_LibraryApplet.hpp>
#include <am/am_HomeMenu.hpp>
#include <am/am_QCommunications.hpp>
#include <util/util_Convert.hpp>
#include <ipc/ipc_IDaemonService.hpp>

extern "C"
{
    u32 __nx_applet_type = AppletType_SystemApplet;
    size_t __nx_heap_size = 0x3000000;//0x1000000;
}

u8 *app_buf;
u128 selected_uid = 0;
hb::TargetInput hblaunch_flag = {};
hb::TargetInput hbapplaunch_copy = {};
hb::TargetInput hbapplaunch_flag = {};
u64 titlelaunch_flag = 0;
WebCommonConfig webapplet_flag = {};

HosMutex latestqlock;
am::QMenuMessage latestqmenumsg = am::QMenuMessage::Invalid;

void CommonSleepHandle()
{
    appletStartSleepSequence(true);
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
                        appletStartSleepSequence(true);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
    svcSleepThread(100000000L);
}

void HandleHomeButton()
{
    bool used_to_reopen_menu = false;
    if(am::LibraryAppletIsActive() && !am::LibraryAppletIsQMenu())
    {
        am::LibraryAppletTerminate();
        am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuHomebrewMode);
        return;
    }
    if(am::ApplicationIsActive())
    {
        if(am::ApplicationHasForeground())
        {
            am::HomeMenuSetForeground();
            am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuApplicationSuspended);
            used_to_reopen_menu = true;
        }
    }
    if(am::LibraryAppletIsQMenu() && !used_to_reopen_menu)
    {
        std::scoped_lock lock(latestqlock);
        latestqmenumsg = am::QMenuMessage::HomeRequest;
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
                appletStartSleepSequence(true);
                break;
            }
            default:
                break;
        }
    }
    svcSleepThread(100000000L);
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
                    selected_uid = reader.Read<u128>();
                    reader.FinishRead();

                    break;
                }
                case am::QDaemonMessage::LaunchApplication:
                {
                    auto app_id = reader.Read<u64>();
                    reader.FinishRead();

                    if(am::ApplicationIsActive())
                    {
                        am::QDaemonCommandResultWriter res(0xBABE1);
                        res.FinishWrite();
                    }
                    else if(selected_uid == 0)
                    {
                        am::QDaemonCommandResultWriter res(0xBABE2);
                        res.FinishWrite();
                    }
                    else if(titlelaunch_flag > 0)
                    {
                        am::QDaemonCommandResultWriter res(0xBABE3);
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
                        am::QDaemonCommandResultWriter res(0xBABE1);
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
                case am::QDaemonMessage::GetSuspendedInfo:
                {
                    reader.FinishRead();
                    
                    am::QSuspendedInfo info = {};
                    cfg::TitleType tmptype = cfg::TitleType::Invalid;
                    if(am::ApplicationIsActive())
                    {
                        tmptype = cfg::TitleType::Installed;
                        if(am::ApplicationGetId() == OS_FLOG_APP_ID) tmptype = cfg::TitleType::Homebrew;
                    }

                    if(tmptype == cfg::TitleType::Installed) info.app_id = am::ApplicationGetId();
                    else if(tmptype == cfg::TitleType::Homebrew) memcpy(&info.input, &hbapplaunch_copy, sizeof(hbapplaunch_copy));

                    am::QDaemonCommandResultWriter writer(0);
                    writer.Write<am::QSuspendedInfo>(info);
                    writer.FinishWrite();

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
                        am::QDaemonCommandResultWriter res(0xBABE1);
                        res.FinishWrite();
                    }
                    else if(selected_uid == 0)
                    {
                        am::QDaemonCommandResultWriter res(0xBABE2);
                        res.FinishWrite();
                    }
                    else if(titlelaunch_flag > 0)
                    {
                        am::QDaemonCommandResultWriter res(0xBABE3);
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
                default:
                    break;
            }
        }
    }
}

namespace qdaemon
{
    void Initialize()
    {
        app_buf = new u8[1280 * 720 * 4]();
        fs::CreateDirectory(Q_BASE_SD_DIR);

        // Debug testing mode
        consoleInit(NULL);
        CONSOLE_FMT("Welcome to QDaemon's debug mode!")
        CONSOLE_FMT("")
        CONSOLE_FMT("(A) -> Dump system save data to sd:/<q>/save_dump")
        CONSOLE_FMT("(B) -> Delete everything in save data (except official HOME menu's content)")
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
                fs::CopyDirectory(Q_DB_MOUNT_NAME ":/", Q_BASE_SD_DIR "/save_dump");
                db::Unmount();
                CONSOLE_FMT(" - Dump done.")
            }
            else if(k & KEY_B)
            {
                db::Mount();
                fs::DeleteDirectory(Q_BASE_DB_DIR);
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

        consoleExit(NULL);

        svcSleepThread(100'000'000); // Wait for proper moment
    }

    void Exit()
    {
        delete[] app_buf;
    }

    void DaemonServiceMain(void *arg)
    {
        static auto server = WaitableManager(2);
        server.AddWaitable(new ServiceServer<ipc::IDaemonService>("qdaemon", 0x10));
        server.Process();
    }

    void ForegroundMain(void *arg)
    {
        u8 *demo = new u8[1280 * 720 * 4]();

        usbCommsInitialize();
        while(true)
        {
            appletUpdateLastForegroundCaptureImage();
            bool tmp;
            appletGetLastForegroundCaptureImageEx(demo, 1280 * 720 * 4, &tmp);
            usbCommsWrite(demo, 1280 * 720 * 4);
        }
        usbCommsExit();
        delete[] demo;
    }

    Result LaunchDaemonServiceThread()
    {
        Thread daemon;
        R_TRY(threadCreate(&daemon, &DaemonServiceMain, NULL, 0x4000, 0x2b, -2));
        R_TRY(threadStart(&daemon));
        return 0;
    }

    Result LaunchForegroundThread()
    {
        Thread fg;
        R_TRY(threadCreate(&fg, &ForegroundMain, NULL, 0x4000, 0x2b, -2));
        R_TRY(threadStart(&fg));
        return 0;
    }
}

int main()
{
    qdaemon::Initialize();
    qdaemon::LaunchDaemonServiceThread();
    qdaemon::LaunchForegroundThread();

    am::QDaemon_LaunchQMenu(am::QMenuStartMode::StartupScreen);

    while(true)
    {
        HandleGeneralChannel();
        HandleAppletMessage();
        HandleQMenuMessage();

        if(webapplet_flag.version > 0) // A valid version in this config is always >= 0x20000
        {
            if(!am::LibraryAppletIsActive())
            {
                am::WebAppletStart(&webapplet_flag);
                svcSleepThread(500'000'000);
                if(!am::LibraryAppletIsActive())
                {
                    // Web applet failed to launch...
                    am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuLaunchFailure);
                }
                memset(&webapplet_flag, 0, sizeof(webapplet_flag));
            }
        }
        if(titlelaunch_flag > 0)
        {
            if(!am::LibraryAppletIsActive())
            {
                am::ApplicationStart(titlelaunch_flag, false, selected_uid);
                svcSleepThread(500'000'000);
                if(!am::ApplicationIsActive())
                {
                    // Title failed to launch, so we re-launch QMenu this way...
                    am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuLaunchFailure);
                }
                titlelaunch_flag = 0;
            }
        }
        if(strlen(hbapplaunch_flag.nro_path))
        {
            if(!am::LibraryAppletIsActive())
            {
                am::ApplicationStart(OS_FLOG_APP_ID, true, selected_uid, &hbapplaunch_flag, sizeof(hbapplaunch_flag));
                svcSleepThread(500'000'000);
                if(!am::ApplicationIsActive())
                {
                    // Title failed to launch, so we re-launch QMenu this way...
                    am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuLaunchFailure);
                }
                hbapplaunch_flag.nro_path[0] = '\0';
            }
        }
        if(strlen(hblaunch_flag.nro_path))
        {
            if(!am::LibraryAppletIsActive())
            {
                am::QDaemon_LaunchQHbTarget(hblaunch_flag);
                svcSleepThread(500'000'000);
                if(!am::LibraryAppletIsActive())
                {
                    // QHbTarget libapplet failed to launch...
                    am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuLaunchFailure);
                }
                hblaunch_flag.nro_path[0] = '\0';
            }
        }
        if(!am::LibraryAppletIsActive())
        {
            switch(am::LibraryAppletGetId())
            {
                case am::QHbTargetAppletId:
                    am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuHomebrewMode);
                    break;
                case AppletId_web:
                    am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuNormal);
                    break;
                default:
                    break;
            }
        }
        svcSleepThread(10'000'000);
    }

    qdaemon::Exit();

    return 0;
}