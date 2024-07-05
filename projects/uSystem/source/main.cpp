#include <ul/system/ecs/ecs_ExternalContent.hpp>
#include <ul/system/sf/sf_IpcManager.hpp>
#include <ul/system/smi/smi_SystemProtocol.hpp>
#include <ul/system/sys/sys_SystemApplet.hpp>
#include <ul/system/system_Message.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/menu/menu_Entries.hpp>
#include <ul/menu/menu_Cache.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/os/os_Applications.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/util/util_Size.hpp>
#include <ul/fs/fs_Stdio.hpp>
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

ul::RecursiveMutex g_MenuMessageQueueLock;
std::queue<ul::smi::MenuMessageContext> *g_MenuMessageQueue;

namespace {

    constexpr AppletId UsedLibraryAppletList[] = {
        AppletId_LibraryAppletPhotoViewer,
        AppletId_LibraryAppletWeb,
        AppletId_LibraryAppletMyPage,
        AppletId_LibraryAppletMiiEdit,
        AppletId_LibraryAppletPlayerSelect,
        AppletId_LibraryAppletNetConnect
    };

    inline bool IsUsedLibraryApplet(const AppletId applet_id) {
        for(u32 i = 0; i < std::size(UsedLibraryAppletList); i++) {
            if(applet_id == UsedLibraryAppletList[i]) {
                return true;
            }
        }

        return false;
    }

    AccountUid g_SelectedUser = {};
    ul::loader::TargetInput g_LoaderLaunchFlag = {};
    bool g_LoaderChooseFlag = false;
    ul::loader::TargetInput g_LoaderApplicationLaunchFlag = {};
    ul::loader::TargetInput g_LoaderApplicationLaunchFlagCopy = {};
    u64 g_ApplicationLaunchFlag = 0;
    WebCommonConfig g_WebAppletLaunchFlag = {};
    bool g_AlbumAppletLaunchFlag = false;
    bool g_UserPageAppletLaunchFlag = false;
    bool g_MiiEditAppletLaunchFlag = false;
    bool g_AddUserAppletLaunchFlag = false;
    bool g_NetConnectAppletLaunchFlag = false;
    bool g_NextMenuLaunchStartupFlag = false;
    bool g_NextMenuLaunchSettingsFlag = false;
    bool g_MenuRestartReloadThemeCacheFlag = false;
    bool g_MenuRestartFlag = false;
    bool g_LoaderOpenedAsApplication = false;
    bool g_AppletActive = false;
    AppletOperationMode g_OperationMode;
    ul::cfg::Config g_Config;

    char g_CurrentMenuFsPath[FS_MAX_PATH] = {};
    char g_CurrentMenuPath[FS_MAX_PATH] = {};
    u32 g_CurrentMenuIndex = 0;

    alignas(ams::os::MemoryPageSize) constinit u8 g_EventManagerThreadStack[16_KB];
    Thread g_EventManagerThread;

    enum class UsbMode : u32 {
        Invalid,
        Rgba,
        Jpeg
    };

    struct UsbPacketHeader {
        UsbMode mode;
        union {
            struct {
            } rgba;
            struct {
                u32 size;
            } jpeg;
        };
    };
    static_assert(sizeof(UsbPacketHeader) == 0x8);

    constexpr size_t PlainRgbaScreenBufferSize = 1280 * 720 * sizeof(u32);
    constexpr size_t UsbPacketSize = sizeof(UsbPacketHeader) + PlainRgbaScreenBufferSize;

    alignas(ams::os::ThreadStackAlignment) constinit u8 g_UsbViewerReadThreadStack[16_KB];
    Thread g_UsbViewerReadThread;
    alignas(ams::os::ThreadStackAlignment) constinit u8 g_UsbViewerWriteThreadStack[16_KB];
    Thread g_UsbViewerWriteThread;
    RwLock g_UsbRwLock;
    UsbPacketHeader *g_UsbViewerBuffer = nullptr;
    u8 *g_UsbViewerBufferDataOffset = nullptr;

    std::vector<NsApplicationRecord> g_CurrentRecords;

    constexpr size_t LibstratosphereHeapSize = 4_MB;
    alignas(ams::os::MemoryPageSize) constinit u8 g_LibstratosphereHeap[LibstratosphereHeapSize];

    constexpr size_t LibnxHeapSize = 4_MB;
    alignas(ams::os::MemoryPageSize) constinit u8 g_LibnxHeap[LibnxHeapSize];

}

namespace {

    void LoadConfig() {
        g_Config = ul::cfg::LoadConfig();

        u64 menu_program_id;
        UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::MenuTakeoverProgramId, menu_program_id));
        la::SetMenuProgramId(menu_program_id);
    }

    inline void PushMenuMessageContext(const ul::smi::MenuMessageContext msg_ctx) {
        ul::ScopedLock lk(g_MenuMessageQueueLock);
        g_MenuMessageQueue->push(msg_ctx);
    }

    inline void PushSimpleMenuMessage(const ul::smi::MenuMessage msg) {
        ul::ScopedLock lk(g_MenuMessageQueueLock);
        const ul::smi::MenuMessageContext msg_ctx = {
            .msg = msg
        };
        g_MenuMessageQueue->push(msg_ctx);
    }

    ul::smi::SystemStatus CreateStatus() {
        ul::smi::SystemStatus status = {
            .selected_user = g_SelectedUser,
            .last_menu_index = g_CurrentMenuIndex
        };

        if(g_MenuRestartReloadThemeCacheFlag) {
            status.reload_theme_cache = true;
            g_MenuRestartReloadThemeCacheFlag = false;
        }

        ul::util::CopyToStringBuffer(status.last_menu_fs_path, g_CurrentMenuFsPath);
        ul::util::CopyToStringBuffer(status.last_menu_path, g_CurrentMenuPath);

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

    inline void CacheAccounts() {
        // Do it this way to avoid wasting a service session all the time
        UL_RC_ASSERT(accountInitialize(AccountServiceType_System));
        UL_RC_ASSERT(ul::acc::CacheAccounts());
        accountExit();
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
            UL_RC_ASSERT(la::Terminate());
            UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::MainMenu, CreateStatus()));
        }
        else if(app::IsActive() && app::HasForeground()) {
            // Hide the application currently on focus and open our menu
            UL_RC_ASSERT(sys::SetForeground());
            UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::MainMenuApplicationSuspended, CreateStatus()));
        }
        else if(la::IsMenu()) {
            // Send a message to our menu to handle itself the home press
            PushSimpleMenuMessage(ul::smi::MenuMessage::HomeRequest);
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
            UL_LOG_INFO("SystemAppletMessageHeader [size: 0x%lX] { magic: 0x%X, unk: 0x%X, msg: %d, unk_2: 0x%X }", sams_st_size, sams_header.magic, sams_header.unk, static_cast<u32>(sams_header.msg), sams_header.unk_2);
            if(sams_header.IsValid()) {
                switch(sams_header.msg) {
                    case GeneralChannelMessage::Unk_Invalid: {
                        UL_LOG_WARN("Invalid general channel message!");
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
                        UL_LOG_WARN("Unimplemented: RequestJumpToSystemUpdate");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayBrightValueChanged: {
                        UL_LOG_WARN("Unimplemented: Unk_OverlayBrightValueChanged");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayAutoBrightnessChanged: {
                        UL_LOG_WARN("Unimplemented: Unk_OverlayAutoBrightnessChanged");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayAirplaneModeChanged: {
                        UL_LOG_WARN("Unimplemented: Unk_OverlayAirplaneModeChanged");
                        break;
                    }
                    case GeneralChannelMessage::Unk_HomeButtonHold: {
                        UL_LOG_WARN("Unimplemented: Unk_HomeButtonHold");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayHidden: {
                        UL_LOG_WARN("Unimplemented: Unk_OverlayHidden");
                        break;
                    }
                    case GeneralChannelMessage::RequestToLaunchApplication: {
                        // TODO (low priority): enum?
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

                        UL_LOG_WARN("Unimplemented: RequestToLaunchApplication { launch_app_request_sender: %d, app_id: 0%16lX, uid: %016lX + %016lX, launch params buf size: 0x%X }", launch_app_request_sender, app_id, uid.uid[0], uid.uid[1], launch_params_buf_size);

                        delete[] launch_params_buf;
                        break;
                    }
                    case GeneralChannelMessage::RequestJumpToStory: {
                        AccountUid uid;
                        UL_RC_ASSERT(sams_st_reader.Read(uid));

                        u64 app_id;
                        UL_RC_ASSERT(sams_st_reader.Read(app_id));

                        UL_LOG_WARN("Unimplemented: RequestJumpToStory { uid: %016lX + %016lX, app_id: 0%16lX }", uid.uid[0], uid.uid[1], app_id);
                        break;
                    }
                    default:
                        // TODO (long term): try to find and implement more messages (mostly those sent by applets!)
                        UL_LOG_WARN("Unhandled general channel message!");
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
            /*
            Applet messages known to be received by us:
            - ChangeIntoForeground
            - ChangeIntoBackground
            - ApplicationExited
            - AutoPowerDown
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
                    if(la::IsMenu()) {
                        PushSimpleMenuMessage(ul::smi::MenuMessage::SdCardEjected);
                    }
                    else {
                        // Power off, since uMenu's UI relies on the SD card, so trying to use uMenu without the SD is not possible at all without any caching...
                        // TODO (low priority): consider handling this in a better way?
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
                    UL_LOG_WARN("Unimplemented applet message: %d", raw_msg);
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
                                return ul::ResultApplicationActive;
                            }
                            if(!accountUidIsValid(&g_SelectedUser)) {
                                return ul::ResultInvalidSelectedUser;
                            }
                            if(g_ApplicationLaunchFlag > 0) {
                                return ul::ResultAlreadyQueued;
                            }

                            g_ApplicationLaunchFlag = launch_app_id;
                            break;
                        }
                        case ul::smi::SystemMessage::ResumeApplication: {
                            if(!app::IsActive()) {
                                return ul::ResultApplicationNotActive;
                            }

                            UL_RC_TRY(app::SetForeground());
                            break;
                        }
                        case ul::smi::SystemMessage::TerminateApplication: {
                            UL_RC_TRY(app::Terminate());
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
                                return ul::ResultApplicationActive;
                            }
                            if(!accountUidIsValid(&g_SelectedUser)) {
                                return ul::ResultInvalidSelectedUser;
                            }
                            if(g_ApplicationLaunchFlag > 0) {
                                return ul::ResultAlreadyQueued;
                            }

                            u64 hb_application_takeover_program_id;
                            UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, hb_application_takeover_program_id));
                            if(hb_application_takeover_program_id == 0) {
                                return ul::ResultNoHomebrewTakeoverApplication;
                            }

                            g_LoaderApplicationLaunchFlag = temp_ipt;
                            g_LoaderApplicationLaunchFlagCopy = temp_ipt;

                            g_ApplicationLaunchFlag = hb_application_takeover_program_id;
                            break;
                        }
                        case ul::smi::SystemMessage::ChooseHomebrew: {
                            g_LoaderLaunchFlag = ul::loader::TargetInput::Create(ul::HbmenuPath, ul::HbmenuPath, true, "Choose a homebrew for uMenu");
                            g_LoaderChooseFlag = true;
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
                            UL_RC_TRY(reader.Pop(g_MenuRestartReloadThemeCacheFlag));
                            g_MenuRestartFlag = true;
                            break;
                        }
                        case ul::smi::SystemMessage::ReloadConfig: {
                            LoadConfig();
                            break;
                        }
                        case ul::smi::SystemMessage::UpdateMenuPaths: {
                            char menu_fs_path[FS_MAX_PATH];
                            UL_RC_TRY(reader.PopData(menu_fs_path, sizeof(menu_fs_path)));

                            char menu_path[FS_MAX_PATH];
                            UL_RC_TRY(reader.PopData(menu_path, sizeof(menu_path)));

                            ul::util::CopyToStringBuffer(g_CurrentMenuFsPath, menu_fs_path);
                            ul::util::CopyToStringBuffer(g_CurrentMenuPath, menu_path);
                            break;
                        }
                        case ul::smi::SystemMessage::UpdateMenuIndex: {
                            u32 menu_index;
                            UL_RC_TRY(reader.Pop(menu_index));

                            g_CurrentMenuIndex = menu_index;
                            break;
                        }
                        case ul::smi::SystemMessage::OpenUserPage: {
                            g_UserPageAppletLaunchFlag = true;
                            break;
                        }
                        case ul::smi::SystemMessage::OpenMiiEdit: {
                            g_MiiEditAppletLaunchFlag = true;
                            break;
                        }
                        case ul::smi::SystemMessage::OpenAddUser: {
                            g_AddUserAppletLaunchFlag = true;
                            break;
                        }
                        case ul::smi::SystemMessage::OpenNetConnect: {
                            g_NetConnectAppletLaunchFlag = true;
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
                        case ul::smi::SystemMessage::ReloadConfig: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::OpenUserPage: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::OpenMiiEdit: {
                            // ...
                            break;
                        }
                        case ul::smi::SystemMessage::OpenAddUser: {
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
                UL_RC_ASSERT(la::StartWeb(&g_WebAppletLaunchFlag));

                sth_done = true;
                g_WebAppletLaunchFlag = {};
            }
        }
        if(g_MenuRestartFlag) {
            if(!la::IsActive()) {
                UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::StartupMenu, CreateStatus()));

                sth_done = true;
                g_MenuRestartFlag = false;
            }
        }
        if(g_AlbumAppletLaunchFlag) {
            if(!la::IsActive()) {
                const struct {
                    u8 album_arg;
                } album_data = { AlbumLaArg_ShowAllAlbumFilesForHomeMenu };
                UL_RC_ASSERT(la::Start(AppletId_LibraryAppletPhotoViewer, 0x10000, &album_data, sizeof(album_data)));

                sth_done = true;
                g_AlbumAppletLaunchFlag = false;
            }
        }
        if(g_UserPageAppletLaunchFlag) {
            if(!la::IsActive()) {
                FriendsLaArgHeader arg_hdr = {
                    .type = FriendsLaArgType_ShowMyProfile,
                    .uid = g_SelectedUser
                };

                if(hosversionAtLeast(9,0,0)) {
                    FriendsLaArg arg = {
                        .hdr = arg_hdr
                    };
                    UL_RC_ASSERT(la::Start(AppletId_LibraryAppletMyPage, 0x10000, &arg, sizeof(arg)));
                }
                else {
                    FriendsLaArgV1 arg = {
                        .hdr = arg_hdr
                    };
                    UL_RC_ASSERT(la::Start(AppletId_LibraryAppletMyPage, 0x1, &arg, sizeof(arg)));
                }

                sth_done = true;
                g_UserPageAppletLaunchFlag = false;
            }
        }
        if(g_MiiEditAppletLaunchFlag) {
            if(!la::IsActive()) {
                const auto mii_ver = hosversionAtLeast(10,2,0) ? 0x4 : 0x3;
                MiiLaAppletInput mii_in = {
                    .version = mii_ver,
                    .mode = MiiLaAppletMode_ShowMiiEdit,
                    .special_key_code = MiiSpecialKeyCode_Normal
                };
                UL_RC_ASSERT(la::Start(AppletId_LibraryAppletMiiEdit, -1, &mii_in, sizeof(mii_in)));

                sth_done = true;
                g_MiiEditAppletLaunchFlag = false;
            }
        }
        if(g_AddUserAppletLaunchFlag) {
            if(!la::IsActive()) {
                PselUiSettings psel_ui;
                UL_RC_ASSERT(pselUiCreate(&psel_ui, PselUiMode_UserCreator));

                // Thanks again libnx for not exposing version stuff :P
                if(hosversionAtLeast(6,0,0)) {
                    UL_RC_ASSERT(la::Start(AppletId_LibraryAppletPlayerSelect, 0x20000, &psel_ui, sizeof(psel_ui)));
                }
                else if (hosversionAtLeast(2,0,0)) {
                    UL_RC_ASSERT(la::Start(AppletId_LibraryAppletPlayerSelect, 0x10000, &psel_ui, sizeof(psel_ui)));
                }
                else {
                    UL_RC_ASSERT(la::Start(AppletId_LibraryAppletPlayerSelect, 0x20000, &psel_ui.settings, sizeof(psel_ui.settings)));
                }

                sth_done = true;
                g_AddUserAppletLaunchFlag = false;
                g_NextMenuLaunchStartupFlag = true;
            }
        }
        if(g_NetConnectAppletLaunchFlag) {
            if(!la::IsActive()) {
                u8 in[28] = {};
                // TODO (low priority): 0 = normal, 1 = qlaunch, 2 = starter...? (consider documenting this better, maybe a PR to libnx even)
                *reinterpret_cast<u32*>(in) = 1;
                UL_RC_ASSERT(la::Start(AppletId_LibraryAppletNetConnect, 0, &in, sizeof(in)));

                /* TODO: send this to uMenu? is it really needed, since it will reload anyway?
                    u8 out[8] = {0};
                    rc = *reinterpret_cast<Result*>(out);
                */

                sth_done = true;
                g_NetConnectAppletLaunchFlag = false;
                g_NextMenuLaunchSettingsFlag = true;
            }
        }
        if(g_ApplicationLaunchFlag > 0) {
            if(!la::IsActive()) {
                // Ensure the application is launchable
                UL_RC_ASSERT(nsTouchApplication(g_ApplicationLaunchFlag));

                if(g_LoaderApplicationLaunchFlag.magic == ul::loader::TargetInput::Magic) {
                    UL_RC_ASSERT(ecs::RegisterLaunchAsApplication(g_ApplicationLaunchFlag, "/ulaunch/bin/uLoader/application", &g_LoaderApplicationLaunchFlag, sizeof(g_LoaderApplicationLaunchFlag), g_SelectedUser));

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

                // TODO (new): consider not asserting and sending the error result to menu instead? same for various other asserts in this code...
                UL_RC_ASSERT(ecs::RegisterLaunchAsApplet(hb_applet_takeover_program_id, 0, "/ulaunch/bin/uLoader/applet", &g_LoaderLaunchFlag, sizeof(g_LoaderLaunchFlag)));
                
                sth_done = true;
                g_LoaderLaunchFlag = {};
            }
        }
        if(!la::IsActive()) {
            const auto cur_id = la::GetLastAppletId();
            u64 hb_applet_takeover_program_id;
            UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::HomebrewAppletTakeoverProgramId, hb_applet_takeover_program_id));
            if(IsUsedLibraryApplet(cur_id) || (cur_id == la::GetAppletIdForProgramId(hb_applet_takeover_program_id))) {
                // A user may have been created...
                if(cur_id == AppletId_LibraryAppletPlayerSelect) {
                    CacheAccounts();
                }

                ul::loader::TargetOutput target_opt;
                if(g_LoaderChooseFlag) {
                    AppletStorage target_opt_st;
                    UL_RC_ASSERT(la::Pop(&target_opt_st));
                    UL_RC_ASSERT(appletStorageRead(&target_opt_st, 0, &target_opt, sizeof(target_opt)));
                }

                auto menu_start_mode = ul::smi::MenuStartMode::MainMenu;
                if(g_NextMenuLaunchStartupFlag) {
                    menu_start_mode = ul::smi::MenuStartMode::StartupMenu;
                    g_NextMenuLaunchStartupFlag = false;
                }
                if(g_NextMenuLaunchSettingsFlag) {
                    menu_start_mode = ul::smi::MenuStartMode::SettingsMenu;
                    g_NextMenuLaunchSettingsFlag = false;
                }
                UL_RC_ASSERT(LaunchMenu(menu_start_mode, CreateStatus()));

                if(g_LoaderChooseFlag) {
                    ul::smi::MenuMessageContext msg_ctx = {
                        .msg = ul::smi::MenuMessage::ChosenHomebrew,
                        .chosen_hb = {}
                    };
                    memcpy(msg_ctx.chosen_hb.nro_path, target_opt.nro_path, sizeof(msg_ctx.chosen_hb.nro_path));
                    PushMenuMessageContext(msg_ctx);

                    g_LoaderChooseFlag = false;
                }
                
                sth_done = true;
            }
        }

        const auto prev_applet_active = g_AppletActive;
        g_AppletActive = la::IsActive();
        if(!sth_done && !prev_applet_active) {
            // If nothing was done but nothing is active, an application or applet might have crashed, terminated, failed to launch...
            if(!app::IsActive() && !la::IsActive()) {
                auto terminate_rc = ul::ResultSuccess;
                if(R_SUCCEEDED(nsGetApplicationTerminateResult(app::GetId(), &terminate_rc))) {
                    UL_LOG_WARN("Application 0x%016X terminated with result %s", app::GetId(), ul::util::FormatResultDisplay(terminate_rc).c_str());
                }

                // Reopen uMenu, notify failure
                UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::MainMenu, CreateStatus()));
                PushSimpleMenuMessage(ul::smi::MenuMessage::PreviousLaunchFailure);
                g_LoaderOpenedAsApplication = false;
            }
        }

        svcSleepThread(10'000'000ul);
    }

    std::vector<NsApplicationRecord> ListChangedRecords() {
        auto old_records = g_CurrentRecords;
        g_CurrentRecords = ul::os::ListApplicationRecords();
        auto new_records = g_CurrentRecords;

        std::vector<NsApplicationRecord> diff_records;
        u32 old_i = 0;
        u32 new_i = 0;
        while(true) {
            if(old_records.at(old_i).application_id == new_records.at(new_i).application_id) {
                old_records.erase(old_records.begin() + old_i);
                new_records.erase(new_records.begin() + new_i);
            }
            else {
                u32 new_j = new_i;
                while(true) {
                    if(old_records.at(old_i).application_id == new_records.at(new_j).application_id) {
                        old_records.erase(old_records.begin() + old_i);
                        new_records.erase(new_records.begin() + new_j);
                        break;
                    }

                    new_j++;
                    if(new_j >= new_records.size()) {
                        diff_records.push_back(old_records.at(old_i));
                        old_records.erase(old_records.begin() + old_i);
                        break;
                    }
                }
            }

            if(old_i >= old_records.size()) {
                break;
            }
            if(new_i >= new_records.size()) {
                break;
            }
        }

        diff_records.insert(diff_records.end(), old_records.begin(), old_records.end());
        diff_records.insert(diff_records.end(), new_records.begin(), new_records.end());
        return diff_records;
    }

    void EventManagerMain(void*) {
        UL_LOG_INFO("EventManager: alive!");
        
        Event record_ev;
        UL_RC_ASSERT(nsGetApplicationRecordUpdateSystemEvent(&record_ev));

        Event gc_mount_fail_event;
        UL_RC_ASSERT(nsGetGameCardMountFailureEvent(&gc_mount_fail_event));

        s32 ev_idx;
        while(true) {
            if(R_SUCCEEDED(waitMulti(&ev_idx, UINT64_MAX, waiterForEvent(&record_ev), waiterForEvent(&gc_mount_fail_event)))) {
                if(ev_idx == 0) {
                    UL_LOG_INFO("Application records changed! diff:");

                    const auto diff_records = ListChangedRecords();
                    for(const auto &record: diff_records) {
                        if(std::find_if(g_CurrentRecords.begin(), g_CurrentRecords.end(), [&](const NsApplicationRecord &rec) -> bool {
                            return rec.application_id == record.application_id;
                        }) != g_CurrentRecords.end()) {
                            UL_LOG_INFO("- [new] 0x%lX", record.application_id);
                            ul::menu::CacheSingleApplication(record.application_id);
                            ul::menu::EnsureApplicationEntry(record);
                        }
                        else {
                            UL_LOG_INFO("- [del] 0x%lX", record.application_id);
                            ul::menu::DeleteApplicationEntry(record.application_id, ul::MenuPath);
                        }
                    }
                }
                if(ev_idx == 1) {
                    eventClear(&gc_mount_fail_event);

                    const auto fail_rc = nsGetLastGameCardMountFailureResult();
                    UL_LOG_INFO("Gamecard mount failed with rc: 0x%X (sending it to uMenu...)", fail_rc);

                    const ul::smi::MenuMessageContext msg_ctx = {
                        .msg = ul::smi::MenuMessage::GameCardMountFailure,
                        .gc_mount_failure = {
                            .mount_rc = fail_rc
                        }
                    };
                    PushMenuMessageContext(msg_ctx);
                }
            }

            svcSleepThread(100'000ul);
        }
    }

    inline size_t CaptureJpegScreenshot() {
        u64 size;
        const auto rc = capsscCaptureJpegScreenShot(&size, g_UsbViewerBufferDataOffset, PlainRgbaScreenBufferSize, ViLayerStack_Default, UINT64_MAX);
        if(R_SUCCEEDED(rc)) {
            return size;
        }
        else {
            return 0;
        }
    }

    void UsbViewerWriteThread(void*) {
        while(true) {
            rwlockWriteLock(&g_UsbRwLock);
            usbCommsWrite(g_UsbViewerBuffer, UsbPacketSize);
            rwlockWriteUnlock(&g_UsbRwLock);
            svcSleepThread(1'000'000ul);
        }
    }

    void UsbViewerRgbaThread(void*) {
        bool tmp_flag;
        while(true) {
            rwlockReadLock(&g_UsbRwLock);
            appletGetLastForegroundCaptureImageEx(g_UsbViewerBufferDataOffset, PlainRgbaScreenBufferSize, &tmp_flag);
            appletUpdateLastForegroundCaptureImage();
            rwlockReadUnlock(&g_UsbRwLock);
            svcSleepThread(1'000'000ul);
        }
    }

    void UsbViewerJpegThread(void*) {
        while(true) {
            rwlockReadLock(&g_UsbRwLock);
            g_UsbViewerBuffer->jpeg.size = (u32)CaptureJpegScreenshot();
            rwlockReadUnlock(&g_UsbRwLock);
            svcSleepThread(1'000'000ul);
        }
    }

    void Initialize() {
        UL_RC_ASSERT(appletLoadAndApplyIdlePolicySettings());
        UL_RC_ASSERT(UpdateOperationMode());

        // Remove old cache
        ul::fs::DeleteDirectory(ul::OldApplicationCachePath);
        ul::fs::DeleteDirectory(ul::OldHomebrewCachePath);
        ul::fs::DeleteDirectory(ul::OldAccountCachePath);

        ul::fs::CleanDirectory(ul::RootCachePath);

        ul::util::CopyToStringBuffer(g_CurrentMenuFsPath, ul::MenuPath);

        CacheAccounts();

        g_CurrentRecords = ul::os::ListApplicationRecords();
        ul::menu::CacheApplications(g_CurrentRecords);
        ul::menu::CacheHomebrew();

        LoadConfig();

        UL_RC_ASSERT(sf::Initialize());

        UL_RC_ASSERT(threadCreate(&g_EventManagerThread, EventManagerMain, nullptr, g_EventManagerThreadStack, sizeof(g_EventManagerThreadStack), 0x2C, -2));
        UL_RC_ASSERT(threadStart(&g_EventManagerThread));

        bool viewer_usb_enabled;
        UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::ViewerUsbEnabled, viewer_usb_enabled));

        if(viewer_usb_enabled) {
            UL_RC_ASSERT(usbCommsInitialize());
            UL_RC_ASSERT(capsscInitialize());

            g_UsbViewerBuffer = reinterpret_cast<UsbPacketHeader*>(__libnx_aligned_alloc(ams::os::MemoryPageSize, UsbPacketSize));
            memset(g_UsbViewerBuffer, 0, UsbPacketSize);

            void(*thread_read_fn)(void*) = nullptr;
            g_UsbViewerBufferDataOffset = reinterpret_cast<u8*>(g_UsbViewerBuffer) + sizeof(UsbMode) + sizeof(UsbPacketHeader::jpeg);
            if(CaptureJpegScreenshot() > 0) {
                g_UsbViewerBuffer->mode = UsbMode::Jpeg;
                thread_read_fn = &UsbViewerJpegThread;
            }
            else {
                g_UsbViewerBuffer->mode = UsbMode::Rgba;
                g_UsbViewerBufferDataOffset = reinterpret_cast<u8*>(g_UsbViewerBuffer) + sizeof(UsbMode);
                thread_read_fn = &UsbViewerRgbaThread;
                capsscExit();
            }

            UL_RC_ASSERT(threadCreate(&g_UsbViewerReadThread, thread_read_fn, nullptr, g_UsbViewerReadThreadStack, sizeof(g_UsbViewerReadThreadStack), 30, -2));
            UL_RC_ASSERT(threadStart(&g_UsbViewerReadThread));
            UL_RC_ASSERT(threadCreate(&g_UsbViewerWriteThread, &UsbViewerWriteThread, nullptr, g_UsbViewerWriteThreadStack, sizeof(g_UsbViewerWriteThreadStack), 28, -2));
            UL_RC_ASSERT(threadStart(&g_UsbViewerWriteThread));
        }
    }

}

extern "C" {

    extern u8 *fake_heap_start;
    extern u8 *fake_heap_end;

}

// TODO (low priority): stop using Atmosphere-libs?

namespace ams {

    namespace init {

        void InitializeSystemModule() {
            ul::InitializeLogging("uSystem");

            UL_RC_ASSERT(sm::Initialize());

            __nx_applet_type = AppletType_SystemApplet;
            UL_RC_ASSERT(appletInitialize());
            UL_RC_ASSERT(fsInitialize());
            UL_RC_ASSERT(nsInitialize());
            UL_RC_ASSERT(ldrShellInitialize());
            UL_RC_ASSERT(pmshellInitialize());
            UL_RC_ASSERT(fsdevMountSdmc());
        }

        void FinalizeSystemModule() {
            capsscExit();
            fsdevUnmountAll();
            pmshellExit();
            ldrShellExit();
            nsExit();
            fsExit();
            appletExit();
        }

        void Startup() {
            // Initialize the global malloc-free/new-delete allocator
            init::InitializeAllocator(g_LibstratosphereHeap, LibstratosphereHeapSize);

            fake_heap_start = g_LibnxHeap;
            fake_heap_end = fake_heap_start + LibnxHeapSize;

            g_MenuMessageQueue = new std::queue<ul::smi::MenuMessageContext>();

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
        UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::StartupMenu, CreateStatus()));

        // Loop forever, since qlaunch should NEVER terminate (AM would crash in that case)
        while(true) {
            MainLoop();
        }
    }

}
