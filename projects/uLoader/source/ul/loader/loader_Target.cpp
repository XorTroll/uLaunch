#include <ul/loader/loader_Target.hpp>
#include <ul/loader/loader_ProgramIdUtils.hpp>
#include <ul/loader/loader_SelfProcess.hpp>
#include <ul/loader/loader_Input.hpp>
#include <ul/loader/loader_HbAbi.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Size.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/util/util_Enum.hpp>
#include <ul/util/util_String.hpp>
#include <cstdlib>

extern "C" {

    u64 g_TargetMapAddress = 0;
    Result g_LastTargetResult = ul::ResultSuccess;

    NX_NORETURN void nroEntrypointTrampoline(const ul::loader::ConfigEntry *entries, u64 handle, u64 entrypoint);

}

using namespace ul::util::size;

namespace ul::loader {

    namespace {

        u8 g_SavedTls[0x100];

        TargetInput g_TargetInput;
        u8 *g_TargetHeapAddress;
        size_t g_TargetHeapSize;
        u32 g_TargetTimes = 0;
        Handle g_SelfProcessHandle;

        ConfigEntry g_TargetConfigEntries[13] = {};
        NroHeader g_TargetHeader = {};

        char g_NextTargetPath[NroPathSize] = {};
        char g_NextTargetArgv[NroArgvSize] = {};

        char g_TargetCurrentArgv[NroArgvSize] = {};

        char g_NoticeText[MenuCaptionSize] = {};

        AccountUid g_UserIdStorage;

        inline void BackupTls() {
            memcpy(g_SavedTls, reinterpret_cast<u8*>(armGetTls()) + 0x100, sizeof(g_SavedTls));
        }

        inline void RestoreTls() {
            memcpy(reinterpret_cast<u8*>(armGetTls()) + 0x100, g_SavedTls, sizeof(g_SavedTls));
        }

        constexpr size_t PageAlignment = 0x1000;

        template<u64 Align>
        inline constexpr u64 AlignUp(const u64 base) {
            return (base + (Align - 1)) &~ (Align - 1);
        }

        inline bool IsKernel4x() {
            u64 dummy = 0;
            const auto rc = svcGetInfo(&dummy, InfoType_InitialProcessIdRange, INVALID_HANDLE, 0);
            return R_VALUE(rc) != KERNELRESULT(InvalidEnumValue);
        }
        
        inline bool IsKernel5xOrLater() {
            u64 dummy = 0;
            const auto rc = svcGetInfo(&dummy, InfoType_UserExceptionContextAddress, INVALID_HANDLE, 0);
            return R_VALUE(rc) != KERNELRESULT(InvalidEnumValue);
        }

        enum class CodeMemoryCapability : u32 {
            Unavailable = 0,
            Available = BIT(0),
            AvailableForSameProcess = BIT(1)
        };
        UL_UTIL_ENUM_DEFINE_FLAG_OPERATORS(CodeMemoryCapability, u32)

        CodeMemoryCapability DetermineCodeMemoryCapability(u8 *heap_addr) {
            if(detectMesosphere()) {
                // Mesosphère allows for same-process code memory usage
                return CodeMemoryCapability::Available | CodeMemoryCapability::AvailableForSameProcess;
            }
            else if(IsKernel5xOrLater()) {
                // On [5.0.0+], the kernel does not allow the creator process of a CodeMemory object to use svcControlCodeMemory on itself, thus returning InvalidMemoryState (0xD401)
                // However the kernel can be patched to support same-process usage of CodeMemory
                // We can detect that by passing a bad operation and observe if we actually get InvalidEnumValue (0xF001)
                Handle code;
                if(R_SUCCEEDED(svcCreateCodeMemory(&code, heap_addr, 0x1000))) {
                    const auto rc = svcControlCodeMemory(code, (CodeMapOperation)-1, 0, 0x1000, 0);
                    svcCloseHandle(code);

                    if(R_VALUE(rc) == KERNELRESULT(InvalidEnumValue)) {
                        return CodeMemoryCapability::Available | CodeMemoryCapability::AvailableForSameProcess;
                    }
                    else {
                        return CodeMemoryCapability::Available;
                    }
                }
            }
            else if(IsKernel4x()) {
                // On [4.0.0-4.1.0] there is no such restriction on same-process CodeMemory usage
                return CodeMemoryCapability::Available | CodeMemoryCapability::AvailableForSameProcess;
            }
            
            return CodeMemoryCapability::Unavailable;
        }

        u64 ComputeMaximumHeapSize(const bool is_auto_gameplay_recording) {
            u64 size = 0;

            u64 available_mem = 0;
            svcGetInfo(&available_mem, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
            u64 used_mem = 0;
            svcGetInfo(&used_mem, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);

            if(available_mem > (used_mem + 2_MB)) {
                size = (available_mem - used_mem - 2_MB) & ~(2_MB - 1);
            }

            if(size == 0) {
                size = 512_MB;
            }

            if((size > 96_MB) && is_auto_gameplay_recording) {
                size -= 96_MB;
            }

            return size;
        }

        Result SetupTargetHeap(const bool is_auto_gameplay_recording, const u64 applet_heap_size, const u64 applet_heap_reservation_size, u8 *&out_heap_addr, u64 &out_heap_size) {
            void *heap_addr = nullptr;
            auto heap_size = ComputeMaximumHeapSize(is_auto_gameplay_recording);

            if(SelfIsApplet()) {
                if(applet_heap_size > 0) {
                    const auto requested_heap_size = AlignUp<2_MB>(applet_heap_size);
                    if(heap_size > requested_heap_size) {
                        heap_size = requested_heap_size;
                    }
                }
                else if(applet_heap_reservation_size > 0) {
                    const auto reserved_heap_size = AlignUp<2_MB>(applet_heap_reservation_size);
                    if(heap_size > reserved_heap_size) {
                        heap_size -= reserved_heap_size;
                    }
                }
            }

            const auto rc = svcSetHeapSize(&heap_addr, heap_size);
            if(R_FAILED(rc) || (heap_addr == nullptr)) {
                UL_RC_ASSERT(hbloader::ResultAllocateHeapFailure);
            }

            out_heap_addr = reinterpret_cast<u8*>(heap_addr);
            out_heap_size = heap_size;
            return ResultSuccess;
        }

        Result InitializeFsdev() {
            UL_RC_TRY(smInitialize());
            UL_RC_TRY(fsInitialize());
            UL_RC_TRY(fsdevMountSdmc());

            return ResultSuccess;
        }

        void FinalizeFsdev() {
            fsdevUnmountAll();
            fsExit();
            smExit();
        }

        NX_NORETURN void LoadTarget() {
            RestoreTls();

            if(g_TargetMapAddress > 0) {
                // Unmap previous target
                const auto rw_size = AlignUp<PageAlignment>(g_TargetHeader.segments[2].size + g_TargetHeader.bss_size);

                if(R_FAILED(svcUnmapProcessCodeMemory(g_SelfProcessHandle, g_TargetMapAddress + g_TargetHeader.segments[0].file_off, reinterpret_cast<u64>(g_TargetHeapAddress) + g_TargetHeader.segments[0].file_off, g_TargetHeader.segments[0].size))) {
                    UL_RC_ASSERT(hbloader::ResultUnmapTextFailure);
                }

                if(R_FAILED(svcUnmapProcessCodeMemory(g_SelfProcessHandle, g_TargetMapAddress + g_TargetHeader.segments[1].file_off, reinterpret_cast<u64>(g_TargetHeapAddress) + g_TargetHeader.segments[1].file_off, g_TargetHeader.segments[1].size))) {
                    UL_RC_ASSERT(hbloader::ResultUnmapRodataFailure);
                }

                if(R_FAILED(svcUnmapProcessCodeMemory(g_SelfProcessHandle, g_TargetMapAddress + g_TargetHeader.segments[2].file_off, reinterpret_cast<u64>(g_TargetHeapAddress) + g_TargetHeader.segments[2].file_off, rw_size))) {
                    UL_RC_ASSERT(hbloader::ResultUnmapDataBssFailure);
                }

                g_TargetMapAddress = 0;
            }

            if(g_NextTargetPath[0] == '\0') {
                util::CopyToStringBuffer(g_NextTargetPath, g_TargetInput.nro_path);
                util::CopyToStringBuffer(g_NextTargetArgv, g_TargetInput.nro_argv);
            }

            util::CopyToStringBuffer(g_TargetCurrentArgv, g_NextTargetArgv);

            auto target_base = g_TargetHeapAddress;
            auto target_start = reinterpret_cast<NroStart*>(target_base);
            auto target_header = reinterpret_cast<NroHeader*>(target_base + sizeof(NroStart));
            auto rest = target_base + sizeof(NroStart) + sizeof(NroHeader);

            {
                UL_RC_ASSERT(InitializeFsdev());
                util::OnScopeExit exit_fsdev([]() {
                    FinalizeFsdev();
                });

                {
                    auto f = fopen(g_NextTargetPath, "rb");
                    if(f == nullptr) {
                        UL_RC_ASSERT(MAKERESULT(333, 900 + errno));
                        UL_RC_ASSERT(hbloader::ResultNroOpenFailure);
                    }
                    util::OnScopeExit file_close([&]() {
                        fclose(f);
                    });

                    if(fread(target_start, sizeof(NroStart), 1, f) != 1) {
                        UL_RC_ASSERT(hbloader::ResultNroHeaderReadFailure);
                    }
                    if(fread(target_header, sizeof(NroHeader), 1, f) != 1) {
                        UL_RC_ASSERT(hbloader::ResultNroHeaderReadFailure);
                    }

                    if(target_header->magic != NROHEADER_MAGIC) {
                        UL_RC_ASSERT(hbloader::ResultInvalidNroMagic);
                    }

                    const auto rest_size = target_header->size - (sizeof(NroStart) + sizeof(NroHeader));
                    if(fread(rest, rest_size, 1, f) != 1) {
                        UL_RC_ASSERT(hbloader::ResultNroReadFailure);
                    }
                }
            }

            // Reset target path to load the default target next time
            g_NextTargetPath[0] = '\0';

            const auto total_size = AlignUp<PageAlignment>(target_header->size + target_header->bss_size);
            const auto rw_size = AlignUp<PageAlignment>(target_header->segments[2].size + target_header->bss_size);

            for(u32 i = 0; i < 3; i++) {
                if((target_header->segments[i].file_off >= target_header->size) || (target_header->segments[i].size > target_header->size) || ((target_header->segments[i].file_off + target_header->segments[i].size) > target_header->size)) {
                    UL_RC_ASSERT(hbloader::ResultInvalidNroSegments);
                }
            }

            // TODO (hbloader): Detect whether NRO fits into heap or not

            // Make a copy of the header before unmapping it
            g_TargetHeader = *target_header;

            // Map code memory to a new randomized address
            virtmemLock();
            const auto map_addr = reinterpret_cast<u64>(virtmemFindCodeMemory(total_size, 0));
            virtmemUnlock();
            if(R_FAILED(svcMapProcessCodeMemory(g_SelfProcessHandle, map_addr, reinterpret_cast<u64>(target_base), total_size))) {
                UL_RC_ASSERT(hbloader::ResultMapCodeMemoryFailure);
            }

            // Setup .text
            if(R_FAILED(svcSetProcessMemoryPermission(g_SelfProcessHandle, map_addr + g_TargetHeader.segments[0].file_off, g_TargetHeader.segments[0].size, Perm_Rx))) {
                UL_RC_ASSERT(hbloader::ResultMapTextFailure);
            }

            // Setup .rodata
            if(R_FAILED(svcSetProcessMemoryPermission(g_SelfProcessHandle, map_addr + g_TargetHeader.segments[1].file_off, g_TargetHeader.segments[1].size, Perm_R))) {
                UL_RC_ASSERT(hbloader::ResultMapRodataFailure);
            }

            // Setup .data and .bss
            if(R_FAILED(svcSetProcessMemoryPermission(g_SelfProcessHandle, map_addr + g_TargetHeader.segments[2].file_off, rw_size, Perm_Rw))) {
                UL_RC_ASSERT(hbloader::ResultMapDataBssFailure);
            }

            const auto target_size = g_TargetHeader.segments[2].file_off + rw_size;
            const auto target_heap_addr = reinterpret_cast<u64>(g_TargetHeapAddress) + target_size;
            const auto target_heap_size = g_TargetHeapSize - target_size;

            auto syscall_available_hint_1 = UINT64_MAX;
            auto syscall_available_hint_2 = UINT64_MAX;
            auto syscall_available_hint_3 = UINT64_MAX;
            const auto code_mem_capability = DetermineCodeMemoryCapability(g_TargetHeapAddress);
            if(!static_cast<bool>(code_mem_capability & CodeMemoryCapability::Available)) {
                // Revoke access to svcCreateCodeMemory (syscall 0x4B) if it's not available
                syscall_available_hint_2 &= ~(1ul << (0x4B % 0x40));
            }
            if(!static_cast<bool>(code_mem_capability & CodeMemoryCapability::AvailableForSameProcess)) {
                // Revoke access to svcControlCodeMemory (syscall 0x4C) if it's not available for same-process usage
                syscall_available_hint_2 &= ~(1ul << (0x4C % 0x40));
            }

            const ConfigEntry target_cfg_entries[] = {
                {
                    EntryKind::MainThreadHandle,
                    static_cast<u32>(EntryFlags::None), {
                        static_cast<u64>(envGetMainThreadHandle()),
                        0
                    }
                },
                {
                    EntryKind::ProcessHandle,
                    static_cast<u32>(EntryFlags::None), {
                        static_cast<u64>(g_SelfProcessHandle),
                        0
                    }
                },
                {
                    EntryKind::AppletType,
                    static_cast<u32>(EntryFlags::None), {
                        static_cast<u64>(GetSelfAppletType()),
                        static_cast<u64>(SelfIsApplication() ? EntryAppletFlags::ApplicationOverride : EntryAppletFlags::None)
                    }
                },
                {
                    EntryKind::OverrideHeap,
                    static_cast<u32>(EntryFlags::Mandatory), {
                        target_heap_addr,
                        target_heap_size
                    }
                },
                {
                    EntryKind::Argv,
                    static_cast<u32>(EntryFlags::None), {
                        0,
                        reinterpret_cast<u64>(g_TargetCurrentArgv)
                    }
                },
                {
                    EntryKind::NextLoadPath,
                    static_cast<u32>(EntryFlags::None), {
                        reinterpret_cast<u64>(g_NextTargetPath),
                        reinterpret_cast<u64>(g_NextTargetArgv)
                    }
                },
                {
                    EntryKind::LastLoadResult,
                    static_cast<u32>(EntryFlags::None), {
                        static_cast<u64>(g_LastTargetResult),
                        0
                    }
                },
                {
                    EntryKind::SyscallAvailableHint,
                    static_cast<u32>(EntryFlags::None), {
                        syscall_available_hint_1,
                        syscall_available_hint_2
                    }
                },
                {
                    EntryKind::SyscallAvailableHint2,
                    static_cast<u32>(EntryFlags::None), {
                        syscall_available_hint_3,
                        0
                    }
                },
                {
                    EntryKind::RandomSeed,
                    static_cast<u32>(EntryFlags::None), {
                        randomGet64(),
                        randomGet64()
                    }
                },
                {
                    EntryKind::UserIdStorage,
                    static_cast<u32>(EntryFlags::None), {
                        reinterpret_cast<u64>(&g_UserIdStorage),
                        0
                    }
                },
                {
                    EntryKind::HosVersion,
                    static_cast<u32>(EntryFlags::None), {
                        static_cast<u64>(hosversionGet()),
                        0x41544D4F53504852ul // 'ATMOSPHR'
                    }
                },
                {
                    EntryKind::EndOfList,
                    static_cast<u32>(EntryFlags::None), {
                        reinterpret_cast<uintptr_t>(reinterpret_cast<char*>(g_NoticeText)),
                        sizeof(g_NoticeText)
                    }
                }
            };
            static_assert(sizeof(g_TargetConfigEntries) == sizeof(target_cfg_entries));
            memcpy(g_TargetConfigEntries, target_cfg_entries, sizeof(target_cfg_entries));

            g_TargetMapAddress = map_addr;

            svcBreak(BreakReason_NotificationOnlyFlag | BreakReason_PostLoadDll, g_TargetMapAddress, target_size);
            nroEntrypointTrampoline(g_TargetConfigEntries, -1, g_TargetMapAddress);
        }

        NX_NORETURN void LoadTargetImpl() {
            if(g_TargetInput.target_once && (g_TargetTimes >= 1)) {
                auto target_opt = TargetOutput::Create(g_NextTargetPath, g_NextTargetArgv);
                if(strlen(target_opt.nro_argv) == 0) {
                    util::CopyToStringBuffer(target_opt.nro_argv, target_opt.nro_path);
                }

                UL_RC_ASSERT(smInitialize());
                UL_LOG_INFO("Sending target output... '%s' with argv '%s'", target_opt.nro_path, target_opt.nro_argv);
                UL_RC_ASSERT(WriteTargetOutput(target_opt));
                smExit();

                exit(0);
            }

            g_TargetTimes++;
            LoadTarget();
        }
        
    }

}

extern "C" {

    NX_NORETURN void ext_LoadTargetImpl() {
        ul::loader::LoadTargetImpl();
    }

}

namespace ul::loader {

    void Target(const TargetInput &target_ipt, const bool is_auto_gameplay_recording, const u64 applet_heap_size, const u64 applet_heap_reservation_size) {
        util::CopyToStringBuffer(g_NoticeText, target_ipt.menu_caption);
        BackupTls();

        UL_RC_ASSERT(GetSelfProcessHandle(g_SelfProcessHandle));

        g_TargetInput = target_ipt;
        UL_RC_ASSERT(SetupTargetHeap(is_auto_gameplay_recording, applet_heap_size, applet_heap_reservation_size, g_TargetHeapAddress, g_TargetHeapSize));

        LoadTargetImpl();
    }

    void LoadTargetOutput(TargetOutput &out_target_opt) {
        out_target_opt = TargetOutput::Create(g_NextTargetPath, g_NextTargetArgv);

        if(strlen(out_target_opt.nro_argv) == 0) {
            util::CopyToStringBuffer(out_target_opt.nro_argv, out_target_opt.nro_path);
        }
    }

}
