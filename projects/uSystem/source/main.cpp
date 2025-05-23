#include <ul/system/ecs/ecs_ExternalContent.hpp>
#include <ul/system/la/la_Open.hpp>
#include <ul/system/sf/sf_IpcManager.hpp>
#include <ul/system/smi/smi_SystemProtocol.hpp>
#include <ul/system/sys/sys_SystemApplet.hpp>
#include <ul/system/system_Message.hpp>
#include <ul/cfg/cfg_Config.hpp>
#include <ul/menu/menu_Entries.hpp>
#include <ul/menu/menu_Cache.hpp>
#include <ul/acc/acc_Accounts.hpp>
#include <ul/os/os_Applications.hpp>
#include <ul/os/os_System.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/util/util_Size.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <queue>
#include <unordered_set>

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

    constexpr const char ChooseHomebrewCaption[] = "Choose a homebrew for uMenu";

    struct ApplicationVerifyContext {
        static constexpr size_t ThreadStackSize = 64_KB;

        u64 app_id;
        Thread thread;
        alignas(ams::os::ThreadStackAlignment) u8 thread_stack[ThreadStackSize];
        bool finished;

        ApplicationVerifyContext(const u64 app_id) : app_id(app_id), thread(), thread_stack(), finished(false) {}
    };

    enum class ActionType : u32 {
        LaunchApplication,
        LaunchLoader,
        LaunchHomebrewApplication,
        OpenWebPage,
        OpenAlbum,
        RestartMenu,
        OpenUserPage,
        OpenMiiEdit,
        OpenAddUser,
        OpenNetConnect,
        OpenCabinet,
        TerminateMenu,
        OpenControllerKeyRemapping,
    };

    struct Action {
        ActionType type;
        union {
            struct {
                u64 app_id;
            } launch_application;
            struct {
                ul::loader::TargetInput target_input;
                bool choose_mode;
            } launch_loader;
            struct {
                u64 app_id;
                ul::loader::TargetInput app_target_input;
            } launch_homebrew_application;
            struct {
                WebCommonConfig cfg;
            } open_web_page;
            struct {
                NfpLaStartParamTypeForAmiiboSettings type;
            } open_cabinet;
            struct {
                u32 npad_style_set;
                HidNpadJoyHoldType hold_type;
            } open_controller_key_remapping;
        };
    };

    // Global state variables

    std::vector<Action> g_ActionQueue;

    AppletOperationMode g_OperationMode;
    bool g_ExpectsLoaderChooseOutput = false;
    ul::loader::TargetInput g_LastHomebrewApplicationLaunchTarget = {};
    bool g_LastLibraryAppletLaunchedNotMenu = false;
    bool g_NextMenuLaunchAtStartup = false;
    bool g_NextMenuLaunchAtSettings = false;
    bool g_MenuRestartReloadThemeCache = false;
    bool g_IsLibraryAppletActive = false;

    NX_INLINE bool IsMenuRunning() {
        return la::IsActive() && !g_LastLibraryAppletLaunchedNotMenu;
    }

    NX_INLINE bool WasLoaderOpenedAsApplication() {
        return g_LastHomebrewApplicationLaunchTarget.IsValid();
    }

    AccountUid g_SelectedUser = {};
    ul::cfg::Config g_Config;
    bool g_AmsIsEmuMMC = false;
    bool g_WarnedAboutOutdatedTheme = false;

    ul::smi::SystemStatus g_CurrentStatus = {};

    char g_CurrentMenuFsPath[FS_MAX_PATH] = {};
    char g_CurrentMenuPath[FS_MAX_PATH] = {};
    u32 g_CurrentMenuIndex = 0;

    constexpr size_t VerifyWorkBufferSize = 0x100000;
    constexpr size_t VerifyStepWaitTimeNs = 100'000;

    std::vector<ApplicationVerifyContext> *g_ApplicationVerifyContexts;

    std::vector<NsExtApplicationRecord> g_CurrentRecords;
    std::vector<u64> g_LastDeletedApplications;
    std::vector<u64> g_LastAddedApplications;

    alignas(ams::os::ThreadStackAlignment) constinit u8 g_EventManagerThreadStack[16_KB];
    Thread g_EventManagerThread;

    // USB types and globals

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

    // Heap definitions

    constexpr size_t LibstratosphereHeapSize = 4_MB;
    alignas(ams::os::MemoryPageSize) constinit u8 g_LibstratosphereHeap[LibstratosphereHeapSize];

    constexpr size_t LibnxHeapSize = 8_MB;
    alignas(ams::os::MemoryPageSize) constinit u8 g_LibnxHeap[LibnxHeapSize];

}

namespace {

    void LoadConfig() {
        g_Config = ul::cfg::LoadConfig();

        u64 menu_program_id;
        UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::MenuTakeoverProgramId, menu_program_id));
        la::SetMenuProgramId(menu_program_id);
    }

    void PushMenuMessageContext(const ul::smi::MenuMessageContext msg_ctx) {
        ul::ScopedLock lk(g_MenuMessageQueueLock);
        g_MenuMessageQueue->push(msg_ctx);
    }

    void PushSimpleMenuMessage(const ul::smi::MenuMessage msg) {
        ul::ScopedLock lk(g_MenuMessageQueueLock);
        const ul::smi::MenuMessageContext msg_ctx = {
            .msg = msg
        };
        g_MenuMessageQueue->push(msg_ctx);
    }

    void NotifyApplicationDeleted(const u64 app_id) {
        u64 takeover_app_id;
        if(g_Config.GetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, takeover_app_id)) {
            if(takeover_app_id == app_id) {
                g_Config.SetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, ul::os::InvalidApplicationId);
                ul::cfg::SaveConfig(g_Config);
            }
        }
        g_LastDeletedApplications.push_back(app_id);
    }

    void UpdateStatus() {
        g_CurrentStatus = {
            .selected_user = g_SelectedUser,
            .last_menu_index = g_CurrentMenuIndex,
            .warned_about_outdated_theme = g_WarnedAboutOutdatedTheme,
            .last_added_app_count = (u32)g_LastAddedApplications.size(),
            .last_deleted_app_count = (u32)g_LastDeletedApplications.size(),
            .in_verify_app_count = (u32)g_ApplicationVerifyContexts->size()
        };

        if(g_MenuRestartReloadThemeCache) {
            g_CurrentStatus.reload_theme_cache = true;
            g_MenuRestartReloadThemeCache = false;
        }

        ul::util::CopyToStringBuffer(g_CurrentStatus.last_menu_fs_path, g_CurrentMenuFsPath);
        ul::util::CopyToStringBuffer(g_CurrentStatus.last_menu_path, g_CurrentMenuPath);

        if(app::IsActive()) {
            if(WasLoaderOpenedAsApplication()) {
                // Homebrew
                g_CurrentStatus.suspended_hb_target_ipt = g_LastHomebrewApplicationLaunchTarget;
            }
            else {
                // Regular title
                g_CurrentStatus.suspended_app_id = app::GetId();
            }
        }
    }

    void HandleSleep() {
        appletStartSleepSequence(true);
    }

    void LocateApplicationAndSpecialEntries(const std::string &path, std::vector<ul::menu::Entry> &out_app_entries, u32 &rem_special_entry_mask) {
        auto entries = ul::menu::LoadEntries(path);
        for(const auto &entry: entries) {
            if(entry.Is<ul::menu::EntryType::Application>()) {
                out_app_entries.push_back(entry);
            }
            else if(entry.IsSpecial()) {
                // This special entry exists, remove from remaining
                rem_special_entry_mask &= ~BITL(static_cast<u32>(entry.type));
            }
            else if(entry.Is<ul::menu::EntryType::Folder>()) {
                LocateApplicationAndSpecialEntries(ul::fs::JoinPath(path, entry.folder_info.fs_name), out_app_entries, rem_special_entry_mask);
            }
        }
    }

    void CheckApplicationRecordChanges() {
        g_LastDeletedApplications.clear();
        g_LastAddedApplications.clear();

        const auto menu_path = ul::menu::MakeMenuPath(g_AmsIsEmuMMC, g_SelectedUser);

        std::vector<ul::menu::Entry> existing_app_entries;
        u32 rem_special_entry_mask =
            BITL(static_cast<u32>(ul::menu::EntryType::SpecialEntryMiiEdit)) |
            BITL(static_cast<u32>(ul::menu::EntryType::SpecialEntryWebBrowser)) |
            BITL(static_cast<u32>(ul::menu::EntryType::SpecialEntryUserPage)) |
            BITL(static_cast<u32>(ul::menu::EntryType::SpecialEntrySettings)) |
            BITL(static_cast<u32>(ul::menu::EntryType::SpecialEntryThemes)) |
            BITL(static_cast<u32>(ul::menu::EntryType::SpecialEntryControllers)) |
            BITL(static_cast<u32>(ul::menu::EntryType::SpecialEntryAlbum)) |
            BITL(static_cast<u32>(ul::menu::EntryType::SpecialEntryAmiibo));

        LocateApplicationAndSpecialEntries(menu_path, existing_app_entries, rem_special_entry_mask);

        // Ensure all special entries exist
        #define _CHECK_HAS_SPECIAL_ENTRY(type) { \
            if((rem_special_entry_mask & BITL(static_cast<u32>(type))) != 0) { \
                ul::menu::CreateSpecialEntry(menu_path, type); \
            } \
        }

        _CHECK_HAS_SPECIAL_ENTRY(ul::menu::EntryType::SpecialEntryMiiEdit);
        _CHECK_HAS_SPECIAL_ENTRY(ul::menu::EntryType::SpecialEntryWebBrowser);
        _CHECK_HAS_SPECIAL_ENTRY(ul::menu::EntryType::SpecialEntryUserPage);
        _CHECK_HAS_SPECIAL_ENTRY(ul::menu::EntryType::SpecialEntrySettings);
        _CHECK_HAS_SPECIAL_ENTRY(ul::menu::EntryType::SpecialEntryThemes);
        _CHECK_HAS_SPECIAL_ENTRY(ul::menu::EntryType::SpecialEntryControllers);
        _CHECK_HAS_SPECIAL_ENTRY(ul::menu::EntryType::SpecialEntryAlbum);
        _CHECK_HAS_SPECIAL_ENTRY(ul::menu::EntryType::SpecialEntryAmiibo);

        // Check applications

        for(auto &app_entry: existing_app_entries) {
            if(std::find_if(g_CurrentRecords.begin(), g_CurrentRecords.end(), [app_entry](const NsExtApplicationRecord &rec) -> bool {
                return rec.id == app_entry.app_info.app_id;
            }) == g_CurrentRecords.end()) {
                UL_LOG_INFO("Deleted application 0x%016lX", app_entry.app_info.app_id);
                NotifyApplicationDeleted(app_entry.app_info.app_id);
                ul::menu::Entry rm_entry(app_entry);
                rm_entry.Remove();
            }
        }

        for(const auto &record: g_CurrentRecords) {
            if(std::find_if(existing_app_entries.begin(), existing_app_entries.end(), [record](const ul::menu::Entry &entry) -> bool {
                return entry.app_info.app_id == record.id;
            }) == existing_app_entries.end()) {
                UL_LOG_INFO("Added application 0x%016lX", record.id);
                g_LastAddedApplications.push_back(record.id);
                while(!ul::menu::CacheSingleApplication(record.id)) {
                    UL_LOG_INFO("> Failed to cache, retrying...");
                    svcSleepThread(100'000ul);
                }
                ul::menu::CacheSingleApplication(record.id);

                ul::menu::EnsureApplicationEntry(record, menu_path);
            }
        }
    }

    Result LaunchMenu(const ul::smi::MenuStartMode st_mode) {
        g_LastLibraryAppletLaunchedNotMenu = false;
        UpdateStatus();

        UL_LOG_INFO("Launching uMenu with start mode %d...", static_cast<u32>(st_mode));
        return ecs::RegisterLaunchAsApplet(la::GetMenuProgramId(), static_cast<u32>(st_mode), "/ulaunch/bin/uMenu", std::addressof(g_CurrentStatus), sizeof(g_CurrentStatus));
    }

    void ApplicationVerifyMain(void *ctx_raw) {
        auto ctx = reinterpret_cast<ApplicationVerifyContext*>(ctx_raw);

        // Like qlaunch does, same size and align
        auto verify_buf = new (std::align_val_t(ams::os::MemoryPageSize)) u8[VerifyWorkBufferSize]();
        NsProgressAsyncResult async_rc;
        NsSystemUpdateProgress progress;
        UL_RC_ASSERT(nsRequestVerifyApplication(&async_rc, ctx->app_id, 0x7, verify_buf, VerifyWorkBufferSize));

        Result verify_rc;
        Result verify_detail_rc;
        while(true) {
            const auto rc = nsProgressAsyncResultWait(&async_rc, VerifyStepWaitTimeNs);
            if(rc == ul::svc::ResultTimedOut) {
                // Still not finished
                UL_RC_ASSERT(nsProgressAsyncResultGetProgress(&async_rc, &progress, sizeof(progress)));

                if(progress.total_size > 0) {
                    const auto progress_val = (float)progress.current_size / (float)progress.total_size;
                    UL_LOG_INFO("[Verify-0x%016lX] done: %lld, total: %lld, prog: %.2f%", ctx->app_id, progress.current_size, progress.total_size, progress_val * 100.0f);

                    if(IsMenuRunning()) {
                        const ul::smi::MenuMessageContext menu_ctx = {
                            .msg = ul::smi::MenuMessage::ApplicationVerifyProgress,
                            .app_verify_progress = {
                                .app_id = ctx->app_id,
                                .done = (u64)progress.current_size,
                                .total = (u64)progress.total_size
                            }
                        };
                        PushMenuMessageContext(menu_ctx);
                    }
                }
                else {
                    UL_LOG_INFO("[Verify-0x%016lX] invalid progress...", ctx->app_id);
                }
            }
            else if(R_SUCCEEDED(rc)) {
                // Finished
                verify_rc = nsProgressAsyncResultGet(&async_rc);
                verify_detail_rc = nsProgressAsyncResultGetDetailResult(&async_rc);
                break;
            }
            else {
                // Unexpected
                UL_LOG_WARN("[Verify-0x%016lX] nsProgressAsyncResultWait failed unexpectedly: %s", ctx->app_id, ul::util::FormatResultDisplay(rc).c_str());

                verify_rc = rc;
                verify_detail_rc = rc;
                break;
            }
        }

        ctx->finished = true;

        nsProgressAsyncResultClose(&async_rc);
        delete[] verify_buf;

        if(R_SUCCEEDED(verify_rc) && R_SUCCEEDED(verify_detail_rc)) {
            // Note: qlaunch apparently calls this command after verification succeeds, it resets the app record/view so that it can be launchable now
            nsClearApplicationTerminateResult(ctx->app_id);
        }

        const ul::smi::MenuMessageContext menu_ctx = {
            .msg = ul::smi::MenuMessage::ApplicationVerifyResult,
            .app_verify_rc = {
                .app_id = ctx->app_id,
                .rc = verify_rc,
                .detail_rc = verify_detail_rc
            }
        };
        PushMenuMessageContext(menu_ctx);
    }

    void HandleHomeButton() {
        if(la::IsActive() && !IsMenuRunning()) {
            // A library applet is opened which is not uMenu, close it and open uMenu
            UL_RC_ASSERT(la::Terminate());
            g_LastLibraryAppletLaunchedNotMenu = false;

            UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::MainMenu));
        }
        else if(app::IsActive() && app::HasForeground()) {
            // Hide the application currently on focus and open uMenu
            UL_RC_ASSERT(sys::SetForeground());

            UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::MainMenu));
        }
        else if(IsMenuRunning()) {
            // Send a message to our menu to handle itself the home press
            PushSimpleMenuMessage(ul::smi::MenuMessage::HomeRequest);
        }
    }

    void HandleGeneralChannelMessage() {
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
                        UL_LOG_WARN("Got GeneralChannelMessage: RequestJumpToSystemUpdate");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayBrightValueChanged: {
                        UL_LOG_WARN("Got GeneralChannelMessage: Unk_OverlayBrightValueChanged");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayAutoBrightnessChanged: {
                        UL_LOG_WARN("Got GeneralChannelMessage: Unk_OverlayAutoBrightnessChanged");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayAirplaneModeChanged: {
                        UL_LOG_WARN("Got GeneralChannelMessage: Unk_OverlayAirplaneModeChanged");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayShown: {
                        UL_LOG_WARN("Got GeneralChannelMessage: Unk_OverlayShown");
                        break;
                    }
                    case GeneralChannelMessage::Unk_OverlayHidden: {
                        UL_LOG_WARN("Got GeneralChannelMessage: Unk_OverlayHidden");
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

                        UL_LOG_WARN("Got GeneralChannelMessage: RequestToLaunchApplication { launch_app_request_sender: %d, app_id: 0%16lX, uid: %016lX + %016lX, launch params buf size: 0x%X }", launch_app_request_sender, app_id, uid.uid[0], uid.uid[1], launch_params_buf_size);

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

    void UpdateOperationMode() {
        // Thank you so much libnx for not exposing the actual call to get the mode via IPC :P
        // We're qlaunch, not using appletMainLoop, thus we have to take care of this manually...
        u8 raw_mode = 0;
        UL_RC_ASSERT(serviceDispatchOut(appletGetServiceSession_CommonStateGetter(), 5, raw_mode));
        g_OperationMode = static_cast<AppletOperationMode>(raw_mode);
    }

    void HandleAppletMessage() {
        u32 finished_verify_count = 0;
        for(auto &ctx: *g_ApplicationVerifyContexts) {
            if(ctx.finished) {
                UL_RC_ASSERT(threadWaitForExit(&ctx.thread));
                UL_RC_ASSERT(threadClose(&ctx.thread));
                finished_verify_count++;
            }
        }

        if(finished_verify_count > 0) {
            auto new_end = std::remove_if(g_ApplicationVerifyContexts->begin(), g_ApplicationVerifyContexts->end(), [](const ApplicationVerifyContext &ctx) { return ctx.finished; });
            g_ApplicationVerifyContexts->erase(new_end, g_ApplicationVerifyContexts->end());   
        }

        u32 raw_msg = 0;
        if(R_SUCCEEDED(appletGetMessage(&raw_msg))) {
            switch(static_cast<ul::system::AppletMessage>(raw_msg)) {
                case ul::system::AppletMessage::ChangeIntoForeground: {
                    UL_LOG_INFO("Got AppletMessage: ChangeIntoForeground");
                    break;
                }
                case ul::system::AppletMessage::ChangeIntoBackground: {
                    UL_LOG_INFO("Got AppletMessage: ChangeIntoBackground");
                    break;
                }
                case ul::system::AppletMessage::ApplicationExited: {
                    UL_LOG_INFO("Got AppletMessage: ApplicationExited");
                    break;
                }
                case ul::system::AppletMessage::DetectShortPressingHomeButton: {
                    HandleHomeButton();
                    break;
                }
                case ul::system::AppletMessage::DetectShortPressingPowerButton: {
                    HandleSleep();
                    break;
                }
                case ul::system::AppletMessage::FinishedSleepSequence: {
                    if(IsMenuRunning()) {
                        PushSimpleMenuMessage(ul::smi::MenuMessage::FinishedSleep);
                    }
                    break;
                }
                case ul::system::AppletMessage::AutoPowerDown: {
                    // From auto-sleep functionality
                    HandleSleep();
                    break;
                }
                case ul::system::AppletMessage::OperationModeChanged: {
                    UpdateOperationMode();
                    break;
                }
                case ul::system::AppletMessage::SdCardRemoved: {
                    if(IsMenuRunning()) {
                        PushSimpleMenuMessage(ul::smi::MenuMessage::SdCardEjected);
                    }
                    else {
                        // Power off, since uMenu's UI relies on the SD card, so trying to use uMenu without the SD is not possible at all without any caching...
                        // TODO (low priority): consider handling this in a better way?
                        UL_RC_ASSERT(appletStartShutdownSequence());
                    }
                    break;
                }
                default:
                    UL_LOG_WARN("Unimplemented applet message: %d", raw_msg);
                    break;
            }
        } 
    }

    void HandleMenuMessage() {
        if(IsMenuRunning()) {
            u32 app_list_count;

            // Note: ignoring result since this won't always succeed, and would error if no commands were received
            smi::ReceiveCommand(
                [&](const ul::smi::SystemMessage msg, smi::ScopedStorageReader &reader) -> Result {
                    switch(msg) {
                        case ul::smi::SystemMessage::SetSelectedUser: {
                            UL_RC_TRY(reader.Pop(g_SelectedUser));
                            CheckApplicationRecordChanges();
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

                            g_ActionQueue.push_back({
                                .type = ActionType::LaunchApplication,
                                .launch_application = {
                                    .app_id = launch_app_id
                                }
                            });
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
                            g_LastHomebrewApplicationLaunchTarget = {};
                            break;
                        }
                        case ul::smi::SystemMessage::LaunchHomebrewLibraryApplet: {
                            ul::loader::TargetInput temp_ipt;
                            UL_RC_TRY(reader.Pop(temp_ipt));

                            g_ActionQueue.push_back({
                                .type = ActionType::LaunchLoader,
                                .launch_loader = {
                                    .target_input = temp_ipt,
                                    .choose_mode = false
                                }
                            });
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

                            u64 hb_application_takeover_program_id;
                            UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, hb_application_takeover_program_id));
                            if(hb_application_takeover_program_id == ul::os::InvalidApplicationId) {
                                return ul::ResultNoHomebrewTakeoverApplication;
                            }

                            g_ActionQueue.push_back({
                                .type = ActionType::LaunchHomebrewApplication,
                                .launch_homebrew_application = {
                                    .app_id = hb_application_takeover_program_id,
                                    .app_target_input = temp_ipt
                                }
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::ChooseHomebrew: {
                            g_ActionQueue.push_back({
                                .type = ActionType::LaunchLoader,
                                .launch_loader = {
                                    .target_input = ul::loader::TargetInput::Create(ul::HbmenuPath, ul::HbmenuPath, true, ChooseHomebrewCaption),
                                    .choose_mode = true
                                }
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::OpenWebPage: {
                            char web_url[500] = {};
                            UL_RC_TRY(reader.PopData(web_url, sizeof(web_url)));

                            WebCommonConfig cfg;
                            UL_RC_TRY(webPageCreate(&cfg, web_url));
                            UL_RC_TRY(webConfigSetWhitelist(&cfg, ".*"));
                            g_ActionQueue.push_back({
                                .type = ActionType::OpenWebPage,
                                .open_web_page = {
                                    .cfg = cfg
                                }
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::OpenAlbum: {
                            g_ActionQueue.push_back({
                                .type = ActionType::OpenAlbum
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::RestartMenu: {
                            UL_RC_TRY(reader.Pop(g_MenuRestartReloadThemeCache));
                            if(g_MenuRestartReloadThemeCache) {
                                g_WarnedAboutOutdatedTheme = false;
                            }
                            
                            g_ActionQueue.push_back({
                                .type = ActionType::RestartMenu
                            });
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
                            g_ActionQueue.push_back({
                                .type = ActionType::OpenUserPage
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::OpenMiiEdit: {
                            g_ActionQueue.push_back({
                                .type = ActionType::OpenMiiEdit
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::OpenAddUser: {
                            g_ActionQueue.push_back({
                                .type = ActionType::OpenAddUser
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::OpenNetConnect: {
                            g_ActionQueue.push_back({
                                .type = ActionType::OpenNetConnect
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::ListAddedApplications: {
                            UL_RC_TRY(reader.Pop(app_list_count));
                            break;
                        }
                        case ul::smi::SystemMessage::ListDeletedApplications: {
                            UL_RC_TRY(reader.Pop(app_list_count));
                            break;
                        }
                        case ul::smi::SystemMessage::OpenCabinet: {
                            u8 type;
                            UL_RC_TRY(reader.Pop(type));

                            g_ActionQueue.push_back({
                                .type = ActionType::OpenCabinet,
                                .open_cabinet = {
                                    .type = static_cast<NfpLaStartParamTypeForAmiiboSettings>(type)
                                }
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::StartVerifyApplication: {
                            u64 app_id;
                            UL_RC_TRY(reader.Pop(app_id));

                            auto &ctx = g_ApplicationVerifyContexts->emplace_back(app_id);
                            UL_RC_ASSERT(threadCreate(&ctx.thread, ApplicationVerifyMain, std::addressof(ctx), ctx.thread_stack, ApplicationVerifyContext::ThreadStackSize, 30, -2));
                            UL_RC_ASSERT(threadStart(&ctx.thread));
                            break;
                        }
                        case ul::smi::SystemMessage::ListInVerifyApplications: {
                            UL_RC_TRY(reader.Pop(app_list_count));
                            break;
                        }
                        case ul::smi::SystemMessage::NotifyWarnedAboutOutdatedTheme: {
                            g_WarnedAboutOutdatedTheme = true;
                            break;
                        }
                        case ul::smi::SystemMessage::TerminateMenu: {
                            g_ActionQueue.push_back({
                                .type = ActionType::TerminateMenu
                            });
                            break;
                        }
                        case ul::smi::SystemMessage::OpenControllerKeyRemapping: {
                            u32 npad_style_set;
                            HidNpadJoyHoldType hold_type;
                            UL_RC_TRY(reader.Pop(npad_style_set));
                            UL_RC_TRY(reader.Pop(hold_type));
                            
                            g_ActionQueue.push_back({
                                .type = ActionType::OpenControllerKeyRemapping,
                                .open_controller_key_remapping = {
                                    .npad_style_set = npad_style_set,
                                    .hold_type = hold_type
                                }
                            });
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
                        case ul::smi::SystemMessage::ListAddedApplications: {
                            if(app_list_count > g_LastAddedApplications.size()) {
                                return ul::ResultInvalidApplicationListCount;
                            }

                            for(u32 i = 0; i < app_list_count; i++) {
                                writer.Push(g_LastAddedApplications.at(i));
                            }

                            g_LastAddedApplications.clear();
                            break;
                        }
                        case ul::smi::SystemMessage::ListDeletedApplications: {
                            if(app_list_count > g_LastDeletedApplications.size()) {
                                return ul::ResultInvalidApplicationListCount;
                            }

                            for(u32 i = 0; i < app_list_count; i++) {
                                writer.Push(g_LastDeletedApplications.at(i));
                            }

                            g_LastDeletedApplications.clear();
                            break;
                        }
                        case ul::smi::SystemMessage::ListInVerifyApplications: {
                            if(app_list_count > g_ApplicationVerifyContexts->size()) {
                                return ul::ResultInvalidApplicationListCount;
                            }

                            for(u32 i = 0; i < app_list_count; i++) {
                                writer.Push(g_ApplicationVerifyContexts->at(i).app_id);
                            }
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

    bool HandleAction(Action &action) {
        UL_LOG_INFO("Trying to handle action of type %d in queue", static_cast<u32>(action.type));
        switch(action.type) {
            case ActionType::LaunchApplication: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching application 0x%016lX...", action.launch_application.app_id);
                    UL_RC_ASSERT(app::Start(action.launch_application.app_id, false, g_SelectedUser));
    
                    return true;
                }
                break;
            }
            case ActionType::LaunchLoader: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching homebrew '%s' as library applet (target once: %s)...", action.launch_loader.target_input.nro_path, action.launch_loader.target_input.target_once ? "true" : "false");
                    u64 hb_applet_takeover_program_id;
                    UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::HomebrewAppletTakeoverProgramId, hb_applet_takeover_program_id));
    
                    // TODO (new): consider not asserting and sending the error result to uMenu instead? same for various other asserts in this code...
                    UL_RC_ASSERT(ecs::RegisterLaunchAsApplet(hb_applet_takeover_program_id, 0, "/ulaunch/bin/uLoader/applet", &action.launch_loader.target_input, sizeof(action.launch_loader.target_input)));

                    g_LastLibraryAppletLaunchedNotMenu = true;
                    
                    // This will be used later to know if we should retrieve the output of uLoader or not
                    g_ExpectsLoaderChooseOutput = action.launch_loader.choose_mode;
                    return true;
                }
                break;
            }
            case ActionType::LaunchHomebrewApplication: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching homebrew '%s' over application 0x%016lX (target once: %s)...", action.launch_homebrew_application.app_target_input.nro_path, action.launch_homebrew_application.app_id, action.launch_homebrew_application.app_target_input.target_once ? "true" : "false");
                    UL_RC_ASSERT(ecs::RegisterLaunchAsApplication(action.launch_homebrew_application.app_id, "/ulaunch/bin/uLoader/application", &action.launch_homebrew_application.app_target_input, sizeof(action.launch_homebrew_application.app_target_input), g_SelectedUser));

                    // Store target input of the launched application
                    g_LastHomebrewApplicationLaunchTarget = action.launch_homebrew_application.app_target_input;

                    return true;
                }
                break;
            }
            case ActionType::OpenWebPage: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching Web...");
                    UL_RC_ASSERT(la::OpenWeb(&action.open_web_page.cfg));

                    g_LastLibraryAppletLaunchedNotMenu = true;
                    return true;
                }
                break;
            }
            case ActionType::OpenAlbum: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching PhotoViewer (ShowAllAlbumFilesForHomeMenu)...");
                    UL_RC_ASSERT(la::OpenPhotoViewerAllAlbumFilesForHomeMenu());
    
                    g_LastLibraryAppletLaunchedNotMenu = true;
                    return true;
                }
                break;
            }
            case ActionType::RestartMenu: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Restarting uMenu...");
                    UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::StartupMenuPostBoot));
    
                    return true;
                }
                break;
            }
            case ActionType::OpenUserPage: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching MyPage (MyProfile)...");
                    UL_RC_ASSERT(la::OpenMyPageMyProfile(g_SelectedUser));
    
                    g_LastLibraryAppletLaunchedNotMenu = true;
                    return true;
                }
                break;
            }
            case ActionType::OpenMiiEdit: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching MiiEdit...");
                    UL_RC_ASSERT(la::OpenMiiEdit());
    
                    g_LastLibraryAppletLaunchedNotMenu = true;
                    return true;
                }
                break;
            }
            case ActionType::OpenAddUser: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching PlayerSelect (UserCreator)...");
                    UL_RC_ASSERT(la::OpenPlayerSelectUserCreator());
    
                    g_LastLibraryAppletLaunchedNotMenu = true;
                    g_NextMenuLaunchAtStartup = true;
                    return true;
                }
                break;
            }
            case ActionType::OpenNetConnect: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching NetConnect...");
                    UL_RC_ASSERT(la::OpenNetConnect());

                    g_LastLibraryAppletLaunchedNotMenu = true;
                    g_NextMenuLaunchAtSettings = true;
                    return true;
                }
                break;
            }
            case ActionType::OpenCabinet: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching Cabinet...");
                    UL_RC_ASSERT(la::OpenCabinet(action.open_cabinet.type));
    
                    g_LastLibraryAppletLaunchedNotMenu = true;
                    return true;
                }
                break;
            }
            case ActionType::TerminateMenu: {
                UL_LOG_INFO("Terminating uMenu...");
                UL_RC_ASSERT(la::Terminate());

                return true;
                break;
            }
            case ActionType::OpenControllerKeyRemapping: {
                if(!la::IsActive()) {
                    UL_LOG_INFO("Launching Controller (KeyRemapping)...");
                    UL_RC_ASSERT(la::OpenControllerKeyRemappingForSystem(action.open_controller_key_remapping.npad_style_set, action.open_controller_key_remapping.hold_type));
    
                    g_LastLibraryAppletLaunchedNotMenu = true;
                    return true;
                }
                break;
            }
        }

        UL_LOG_INFO("Failed to handle action in queue");
        return false;
    }

    void MainLoop() {
        HandleGeneralChannelMessage();
        HandleAppletMessage();
        HandleMenuMessage();

        auto consumed_action = false;
        // Try to consume one action per loop
        if(!g_ActionQueue.empty()) {
            UL_LOG_INFO("Action queue has %lu actions", g_ActionQueue.size());
        }
        for(u32 i = 0; i < g_ActionQueue.size(); i++) {
            auto &action = g_ActionQueue.at(i);
            consumed_action |= HandleAction(action);
            if(consumed_action) {
                // Action consumed, remove it from the queue
                g_ActionQueue.erase(g_ActionQueue.begin() + i);
                break;
            }
        }

        auto something_done = consumed_action;

        // Handle finalized active library applet

        if(!la::IsActive() && g_LastLibraryAppletLaunchedNotMenu) {
            // The library applet that we launched and was active just finished, and it's not uMenu --> we either collect output (if needed) or just return to uMenu

            UL_LOG_INFO("Active library applet finished, checking if we need to collect any output before launching uMenu...");

            // If we launched uLoader to choose a homebrew, we need to collect the output
            ul::loader::TargetOutput target_opt;
            if(g_ExpectsLoaderChooseOutput) {
                UL_LOG_INFO("Getting loader chosen output...");

                AppletStorage target_opt_st;
                UL_RC_ASSERT(la::Pop(&target_opt_st));
                UL_RC_ASSERT(appletStorageRead(&target_opt_st, 0, &target_opt, sizeof(target_opt)));
            }

            // Pick correct uMenu state to launch to, and launch uMenu
            auto menu_start_mode = ul::smi::MenuStartMode::MainMenu;
            if(g_NextMenuLaunchAtStartup) {
                menu_start_mode = ul::smi::MenuStartMode::StartupMenu;
                g_NextMenuLaunchAtStartup = false;
            }
            if(g_NextMenuLaunchAtSettings) {
                menu_start_mode = ul::smi::MenuStartMode::SettingsMenu;
                g_NextMenuLaunchAtSettings = false;
            }
            UL_RC_ASSERT(LaunchMenu(menu_start_mode));

            // If we collected output from uLoader, send it to the just-launched uMenu
            if(g_ExpectsLoaderChooseOutput) {
                ul::smi::MenuMessageContext msg_ctx = {
                    .msg = ul::smi::MenuMessage::ChosenHomebrew,
                    .chosen_hb = {}
                };
                memcpy(msg_ctx.chosen_hb.nro_path, target_opt.nro_path, sizeof(msg_ctx.chosen_hb.nro_path));
                PushMenuMessageContext(msg_ctx);

                g_ExpectsLoaderChooseOutput = false;
            }
            
            something_done = true;
        }

        // Final check

        const auto prev_applet_active = g_IsLibraryAppletActive;
        g_IsLibraryAppletActive = la::IsActive();
        if(!something_done && !prev_applet_active) {
            // If nothing was done but nothing is active, an application or library applet might have crashed, terminated, failed to launch...
            if(!app::IsActive() && !la::IsActive()) {
                UL_LOG_INFO("No application or library applet is active, checking for application launch failure...");

                if(hosversionAtLeast(6,0,0)) {
                    auto terminate_rc = ul::ResultSuccess;
                    if(R_SUCCEEDED(nsGetApplicationTerminateResult(app::GetId(), &terminate_rc))) {
                        UL_LOG_WARN("Application 0x%016lX terminated with result %s", app::GetId(), ul::util::FormatResultDisplay(terminate_rc).c_str());
                    }
                }

                g_LastHomebrewApplicationLaunchTarget = {};

                // Reopen uMenu, notify failure
                UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::MainMenu));
                PushSimpleMenuMessage(ul::smi::MenuMessage::PreviousLaunchFailure);
            }
        }

        svcSleepThread(10'000'000ul);
    }

    std::vector<NsExtApplicationRecord> ListAddedRecords(const std::vector<NsExtApplicationRecord> &old_records, const std::vector<NsExtApplicationRecord> &cur_records) {
        std::unordered_set<u64> prev_app_ids;
        for(const auto &rec: old_records) {
            prev_app_ids.insert(rec.id);
        }

        std::vector<NsExtApplicationRecord> added;
        for(const auto &rec: cur_records) {
            if(prev_app_ids.find(rec.id) == prev_app_ids.end()) {
                added.push_back(rec);
            }
        }
        return added;
    }

    std::vector<NsExtApplicationRecord> ListRemovedRecords(const std::vector<NsExtApplicationRecord> &old_records, const std::vector<NsExtApplicationRecord> &cur_records) {
        std::unordered_set<u64> cur_app_ids;
        for(const auto &rec: cur_records) {
            cur_app_ids.insert(rec.id);
        }

        std::vector<NsExtApplicationRecord> removed;
        for(const auto &rec: old_records) {
            if(cur_app_ids.find(rec.id) == cur_app_ids.end()) {
                removed.push_back(rec);
            }
        }
        return removed;
    }

    void EventManagerMain(void*) {
        UL_LOG_INFO("[EventManager] alive!");
        
        Event record_ev;
        UL_RC_ASSERT(nsGetApplicationRecordUpdateSystemEvent(&record_ev));
        UL_LOG_INFO("[EventManager] registered ApplicationRecordUpdateSystemEvent");

        Event gc_mount_fail_event;
        if(hosversionAtLeast(3,0,0)) {
            UL_RC_ASSERT(nsGetGameCardMountFailureEvent(&gc_mount_fail_event));
            UL_LOG_INFO("[EventManager] registered GameCardMountFailureEvent");
        }
        else {
            UL_LOG_INFO("[EventManager] cannot register GameCardMountFailureEvent, unsuported by firmware!");
        }

        s32 ev_idx;
        while(true) {
            auto wait_rc = ul::ResultSuccess;
            if(hosversionAtLeast(3,0,0)) {
                wait_rc = waitMulti(&ev_idx, UINT64_MAX, waiterForEvent(&record_ev), waiterForEvent(&gc_mount_fail_event));
            }
            else {
                wait_rc = waitMulti(&ev_idx, UINT64_MAX, waiterForEvent(&record_ev));
            }

            if(R_SUCCEEDED(wait_rc)) {
                if(ev_idx == 0) {
                    g_LastAddedApplications.clear();
                    g_LastDeletedApplications.clear();
                    UL_LOG_INFO("[EventManager] Application records changed!");

                    std::vector<AccountUid> uids;
                    UL_RC_ASSERT(ul::acc::ListAccounts(uids));

                    const auto old_records = std::move(g_CurrentRecords);
                    g_CurrentRecords = ul::os::ListApplicationRecords();

                    const auto added_records = ListAddedRecords(old_records, g_CurrentRecords);
                    for(const auto &record: added_records) {
                        UL_LOG_INFO("[EventManager] > Added application 0x%016lX, caching...", record.id);
                        g_LastAddedApplications.push_back(record.id);

                        u32 i = 0;
                        while(!ul::menu::CacheSingleApplication(record.id)) {
                            if(i >= 50) {
                                UL_LOG_WARN("[EventManager] > Failed to cache application 0x%016lX 50 times, giving up...", record.id);
                                break;
                            }

                            UL_LOG_INFO("[EventManager] > Failed to cache, retrying...");
                            svcSleepThread(100'000ul);

                            i++;
                        }

                        for(const auto &uid: uids) {
                            const auto menu_path = ul::menu::MakeMenuPath(g_AmsIsEmuMMC, uid);
                            UL_LOG_INFO("[EventManager] > Ensuring application ID 0x%016lX entry in user menu (%s)", record.id, menu_path.c_str());
                            ul::menu::EnsureApplicationEntry(record, menu_path);
                        }
                    }

                    const auto removed_records = ListRemovedRecords(old_records, g_CurrentRecords);
                    for(const auto &record: removed_records) {
                        UL_LOG_INFO("[EventManager] > Deleted application 0x%016lX", record.id);
                        NotifyApplicationDeleted(record.id);

                        for(const auto &uid: uids) {
                            const auto menu_path = ul::menu::MakeMenuPath(g_AmsIsEmuMMC, uid);
                            ul::menu::DeleteApplicationEntryRecursively(record.id, menu_path);
                        }
                    }

                    // Only push this if uMenu is currently active
                    if(IsMenuRunning()) {
                        const ul::smi::MenuMessageContext msg_ctx = {
                            .msg = ul::smi::MenuMessage::ApplicationRecordsChanged,
                            .app_records_changed = {
                                .records_added_or_deleted = !added_records.empty() || !removed_records.empty(),
                            }
                        };
                        PushMenuMessageContext(msg_ctx);
                    }
                }
                else if(ev_idx == 1) {
                    eventClear(&gc_mount_fail_event);

                    const auto fail_rc = nsGetLastGameCardMountFailureResult();
                    UL_LOG_INFO("[EventManager] Gamecard mount failed with rc: 0x%X", fail_rc);

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

    size_t CaptureJpegScreenshot() {
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

    void CheckHomebrewTakeoverApplicationId() {
        u64 hb_application_takeover_program_id;
        UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, hb_application_takeover_program_id));
        if(hb_application_takeover_program_id != ul::os::InvalidApplicationId) {
            // Simple command, not involving buffers (unlike GetApplicationControlData) that will fail if the ID is invalid
            const auto valid_app = R_SUCCEEDED(nsTouchApplication(hb_application_takeover_program_id));
            UL_LOG_INFO("Homebrew take-over application ID: 0x%016lX, valid: %d", hb_application_takeover_program_id, valid_app);
            if(!valid_app) {
                g_Config.SetEntry(ul::cfg::ConfigEntryId::HomebrewApplicationTakeoverApplicationId, ul::os::InvalidApplicationId);
                ul::cfg::SaveConfig(g_Config);
            }
        }
    }

    void Initialize() {
        UL_RC_ASSERT(appletLoadAndApplyIdlePolicySettings());
        UpdateOperationMode();

        g_AmsIsEmuMMC = ul::os::IsEmuMMC();

        // Remove old cache
        ul::fs::DeleteDirectory(ul::PreV100ApplicationCachePath);
        ul::fs::DeleteDirectory(ul::PreV100HomebrewCachePath);
        ul::fs::DeleteDirectory(ul::PreV100AccountCachePath);

        ul::fs::CleanDirectory(ul::RootCachePath);

        ul::menu::SetLoadApplicationEntryVersions(false);
        g_CurrentRecords = ul::os::ListApplicationRecords();
        ul::menu::CacheApplications(g_CurrentRecords);
        ul::menu::CacheHomebrew();
        CheckHomebrewTakeoverApplicationId();

        LoadConfig();

        UL_RC_ASSERT(sf::Initialize());

        UL_RC_ASSERT(threadCreate(&g_EventManagerThread, EventManagerMain, nullptr, g_EventManagerThreadStack, sizeof(g_EventManagerThreadStack), 34, -2));
        UL_RC_ASSERT(threadStart(&g_EventManagerThread));

        bool viewer_usb_enabled;
        UL_ASSERT_TRUE(g_Config.GetEntry(ul::cfg::ConfigEntryId::UsbScreenCaptureEnabled, viewer_usb_enabled));
        if(viewer_usb_enabled) {
            UL_RC_ASSERT(usbCommsInitialize());
            UL_RC_ASSERT(capsscInitialize());

            g_UsbViewerBuffer = reinterpret_cast<UsbPacketHeader*>(__libnx_aligned_alloc(ams::os::MemoryPageSize, UsbPacketSize));
            memset(g_UsbViewerBuffer, 0, UsbPacketSize);

            void(*thread_read_fn)(void*) = nullptr;
            g_UsbViewerBufferDataOffset = reinterpret_cast<u8*>(g_UsbViewerBuffer) + sizeof(UsbMode) + sizeof(UsbPacketHeader::jpeg);
            if(hosversionAtLeast(9,0,0) && (CaptureJpegScreenshot() > 0)) {
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
            __nx_applet_type = AppletType_SystemApplet;

            UL_RC_ASSERT(sm::Initialize());
            UL_RC_ASSERT(fsInitialize());
            
            UL_RC_ASSERT(appletInitialize());
            
            UL_RC_ASSERT(nsInitialize());
            UL_RC_ASSERT(ldrShellInitialize());
            UL_RC_ASSERT(pmshellInitialize());
            UL_RC_ASSERT(setsysInitialize());
            UL_RC_ASSERT(accountInitialize(AccountServiceType_System));

            // FS and log init is intentionally done at the end, otherwise the SD doesn't seem to be always ready...?
            UL_RC_ASSERT(fsdevMountSdmc());
            ul::InitializeLogging("uSystem");
        }

        void FinalizeSystemModule() {
            accountExit();
            setsysExit();
            capsscExit();
            pmshellExit();
            ldrShellExit();
            nsExit();
            fsdevUnmountAll();
            fsExit();
            appletExit();
        }

        void Startup() {
            // Initialize the global malloc-free/new-delete allocator
            init::InitializeAllocator(g_LibstratosphereHeap, LibstratosphereHeapSize);

            fake_heap_start = g_LibnxHeap;
            fake_heap_end = fake_heap_start + LibnxHeapSize;

            g_MenuMessageQueue = new std::queue<ul::smi::MenuMessageContext>();
            g_ApplicationVerifyContexts = new std::vector<ApplicationVerifyContext>();

            os::SetThreadNamePointer(os::GetCurrentThread(), "ul.system.Main");
        }

    }

    NORETURN void Exit(int rc) {
        AMS_UNUSED(rc);
        UL_RC_ASSERT(false && "Unexpected exit called by system applet (uSystem)");
        AMS_ABORT();
    }

    // uSystem handles basic qlaunch functionality since it is the back-end of the project, communicating with uMenu when neccessary

    void Main() {
        // Initialize everything
        Initialize();

        UL_LOG_INFO("Hello World from uSystem! Launching uMenu...");

        // After having initialized everything, launch our menu
        UL_RC_ASSERT(LaunchMenu(ul::smi::MenuStartMode::StartupMenuPostBoot));

        // Loop forever, since qlaunch should NEVER terminate (AM would crash in that case)
        while(true) {
            MainLoop();
        }
    }

}
