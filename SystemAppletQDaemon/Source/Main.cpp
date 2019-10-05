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

extern "C"
{
    u32 __nx_applet_type = AppletType_SystemApplet;
    size_t __nx_heap_size = 0x3000000;//0x1000000;
}

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
                }
            }
        }
    }
    svcSleepThread(100000000L);
}

u8 *app_buf;
u128 selected_uid = 0;
u64 titlelaunch_flag = 0;
bool titlelaunch_system = false;

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
                if(am::ApplicationIsActive())
                {
                    if(am::ApplicationHasForeground())
                    {
                        bool flag;
                        appletUpdateLastForegroundCaptureImage();
                        appletGetLastForegroundCaptureImageEx(app_buf, 1280 * 720 * 4, &flag);
                        FILE *f = fopen(Q_BASE_SD_DIR "/temp-suspended.rgba", "wb");
                        if(f)
                        {
                            fwrite(app_buf, 1, 1280 * 720 * 4, f);
                            fclose(f);
                        }
                        
                        // Get what the user was doing before returning to QMenu
                        am::HomeMenuSetForeground();
                        am::QDaemon_LaunchQMenu(am::QMenuStartMode::MenuApplicationSuspended);
                    }
                }
                else if(am::LibraryAppletIsQMenu())
                {
                    am::QDaemonCommandWriter writer(am::QMenuMessage::HomeRequest);
                }
                break;
            }
            case os::AppletMessage::PowerButton:
            {
                appletStartSleepSequence(true);
                break;
            }
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
                    auto is_system = reader.Read<bool>();
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
                        titlelaunch_system = is_system;
                        am::QDaemonCommandResultWriter res(0);
                        res.FinishWrite();
                    }

                    break;
                }
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
        fs::CreateDirectory(Q_BASE_SD_DIR "/user");
        fs::CreateDirectory(Q_BASE_SD_DIR "/title");
        fs::CreateDirectory(Q_BASE_SD_DIR "/nro");
        fs::CreateDirectory(Q_BASE_SD_DIR "/entries");

        consoleInit(NULL);

        svcSleepThread(500'000'000); // Wait for proper moment
    }

    void Exit()
    {
        delete[] app_buf;
    }
}

int main()
{
    qdaemon::Initialize();

    CONSOLE_OUT("QDaemon - in!");
    am::QDaemon_LaunchQMenu(am::QMenuStartMode::StartupScreen);

    while(true)
    {
        /*
        hidScanInput();
        auto k = hidKeysDown(CONTROLLER_P1_AUTO);
        if(k & KEY_A)
        {
            CONSOLE_OUT("QMenu IsActive? -> " << std::boolalpha << am::LibraryAppletIsActive());
        }
        else if(k & KEY_B)
        {
            nsInitialize();
            auto [rc, tit] = os::QueryInstalledTitles(false);
            nsExit();

            accountInitialize();
            auto [rc2, us] = os::QuerySystemAccounts(false);
            accountExit();

            CONSOLE_OUT("Launching title " << util::FormatApplicationId(tit[0].app_id) << "...");

            rc = am::ApplicationStart(tit[0].app_id, false, us[0]);
            CONSOLE_FMT("am::ApplicationStart(tit[0].app_id, false, us[0]) -> 0x%X", rc);
        }
        */

        HandleGeneralChannel();
        HandleAppletMessage();
        HandleQMenuMessage();

        if(titlelaunch_flag > 0)
        {
            if(!am::LibraryAppletIsActive())
            {
                auto rc = am::ApplicationStart(titlelaunch_flag, titlelaunch_system, selected_uid);
                if(R_FAILED(rc)) am::QDaemon_LaunchQMenu(am::QMenuStartMode::StartupScreen);
                titlelaunch_flag = 0;
            }
        }

        svcSleepThread(10'000'000);
    }

    qdaemon::Exit();

    return 0;
}