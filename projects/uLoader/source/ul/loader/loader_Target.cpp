#include <ul/loader/loader_Target.hpp>
#include <ul/loader/loader_ProgramIdUtils.hpp>
#include <ul/loader/loader_SelfProcess.hpp>
#include <ul/loader/loader_HbAbi.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Size.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/util/util_Enum.hpp>
#include <cstring>
#include <cstdlib>

extern "C" {

    extern u64 __stack_top;

    u64 g_TargetMapAddress = 0;

}

using namespace ul::util::size;

namespace ul::loader {

    namespace {

        u8 g_SavedTls[0x100];
        Result g_LastTargetResult = ResultSuccess;

        ConfigEntry g_TargetConfigEntries[13] = {};
        NroHeader g_TargetHeader = {};

        constexpr size_t InternalPathSize = 512;
        constexpr size_t InternalArgvSize = 2048;

        char g_NextTargetPath[InternalPathSize] = {};
        char g_NextTargetArgv[InternalArgvSize] = {};

        char g_TargetCurrentArgv[InternalArgvSize] = {};

        char g_NoticeText[] = "Loaded by uLoader v" UL_VERSION " - uLaunch's custom hbloader alternative ;)";

        AccountUid g_UserIdStorage;

        template<size_t S1, size_t S2>
        inline void CopyStringBuffer(char (&dst)[S1], const char (&src)[S2]) {
            constexpr auto copy_size = std::min(S1, S2);
            memcpy(dst, src, copy_size);
            dst[copy_size - 1] = '\0';
        }

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
                return MAKERESULT(Module_HomebrewLoader, 9);
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

        Result DoTarget(const TargetInput &target_ipt, u8 *heap_addr, const u64 heap_size) {
            RestoreTls();
            Handle self_proc_handle;
            UL_RC_TRY(GetSelfProcessHandle(self_proc_handle));

            if(g_TargetMapAddress > 0) {
                // Unmap previous target
                const auto rw_size = AlignUp<PageAlignment>(g_TargetHeader.segments[2].size + g_TargetHeader.bss_size);

                if(R_FAILED(svcUnmapProcessCodeMemory(self_proc_handle, g_TargetMapAddress + g_TargetHeader.segments[0].file_off, reinterpret_cast<u64>(heap_addr) + g_TargetHeader.segments[0].file_off, g_TargetHeader.segments[0].size))) {
                    return MAKERESULT(Module_HomebrewLoader, 24);
                }

                if(R_FAILED(svcUnmapProcessCodeMemory(self_proc_handle, g_TargetMapAddress + g_TargetHeader.segments[1].file_off, reinterpret_cast<u64>(heap_addr) + g_TargetHeader.segments[1].file_off, g_TargetHeader.segments[1].size))) {
                    return MAKERESULT(Module_HomebrewLoader, 25);
                }

                if(R_FAILED(svcUnmapProcessCodeMemory(self_proc_handle, g_TargetMapAddress + g_TargetHeader.segments[2].file_off, reinterpret_cast<u64>(heap_addr) + g_TargetHeader.segments[2].file_off, rw_size))) {
                    return MAKERESULT(Module_HomebrewLoader, 26);
                }

                g_TargetMapAddress = 0;
            }

            if(g_NextTargetPath[0] == '\0') {
                CopyStringBuffer(g_NextTargetPath, target_ipt.nro_path);
                CopyStringBuffer(g_NextTargetArgv, target_ipt.nro_argv);
            }

            CopyStringBuffer(g_TargetCurrentArgv, g_NextTargetArgv);

            auto target_base = heap_addr;
            auto target_start = reinterpret_cast<NroStart*>(target_base);
            auto target_header = reinterpret_cast<NroHeader*>(target_base + sizeof(NroStart));
            auto rest = target_base + sizeof(NroStart) + sizeof(NroHeader);

            {
                UL_RC_TRY(InitializeFsdev());
                util::OnScopeExit exit_fsdev([]() {
                    FinalizeFsdev();
                });

                {
                    auto f = fopen(g_NextTargetPath, "rb");
                    if(f == nullptr) {
                        return MAKERESULT(333, errno + 9000);
                        return MAKERESULT(Module_HomebrewLoader, 3);
                    }
                    util::OnScopeExit file_close([&]() {
                        fclose(f);
                    });

                    if(fread(target_start, sizeof(NroStart), 1, f) != 1) {
                        return MAKERESULT(Module_HomebrewLoader, 4);
                    }
                    if(fread(target_header, sizeof(NroHeader), 1, f) != 1) {
                        return MAKERESULT(Module_HomebrewLoader, 4);
                    }

                    if(target_header->magic != NROHEADER_MAGIC) {
                        return MAKERESULT(Module_HomebrewLoader, 5);
                    }

                    const auto rest_size = target_header->size - (sizeof(NroStart) + sizeof(NroHeader));
                    if(fread(rest, rest_size, 1, f) != 1) {
                        return MAKERESULT(Module_HomebrewLoader, 7);
                    }
                }
            }

            // Reset target path to load the default target next time
            g_NextTargetPath[0] = '\0';

            const auto total_size = AlignUp<PageAlignment>(target_header->size + target_header->bss_size);
            const auto rw_size = AlignUp<PageAlignment>(target_header->segments[2].size + target_header->bss_size);

            for(u32 i = 0; i < 3; i++) {
                if((target_header->segments[i].file_off >= target_header->size) || (target_header->segments[i].size > target_header->size) || ((target_header->segments[i].file_off + target_header->segments[i].size) > target_header->size)) {
                    return MAKERESULT(Module_HomebrewLoader, 6);
                }
            }

            // TODO (hbloader): Detect whether NRO fits into heap or not

            // Make a copy of the header before unmapping it
            g_TargetHeader = *target_header;

            // Map code memory to a new randomized address
            virtmemLock();
            const auto map_addr = reinterpret_cast<u64>(virtmemFindCodeMemory(total_size, 0));
            virtmemUnlock();
            if(R_FAILED(svcMapProcessCodeMemory(self_proc_handle, map_addr, reinterpret_cast<u64>(target_base), total_size))) {
                return MAKERESULT(Module_HomebrewLoader, 18);
            }

            // Setup .text
            if(R_FAILED(svcSetProcessMemoryPermission(self_proc_handle, map_addr + g_TargetHeader.segments[0].file_off, g_TargetHeader.segments[0].size, Perm_Rx))) {
                return MAKERESULT(Module_HomebrewLoader, 19);
            }

            // Setup .rodata
            if(R_FAILED(svcSetProcessMemoryPermission(self_proc_handle, map_addr + g_TargetHeader.segments[1].file_off, g_TargetHeader.segments[1].size, Perm_R))) {
                return MAKERESULT(Module_HomebrewLoader, 20);
            }

            // Setup .data and .bss
            if(R_FAILED(svcSetProcessMemoryPermission(self_proc_handle, map_addr + g_TargetHeader.segments[2].file_off, rw_size, Perm_Rw))) {
                return MAKERESULT(Module_HomebrewLoader, 21);
            }

            const auto target_size = g_TargetHeader.segments[2].file_off + rw_size;
            const auto target_heap_addr = reinterpret_cast<u64>(heap_addr) + target_size;
            const auto target_heap_size = heap_size - target_size;

            auto syscall_available_hint_1 = UINT64_MAX;
            auto syscall_available_hint_2 = UINT64_MAX;
            auto syscall_available_hint_3 = UINT64_MAX;
            const auto code_mem_capability = DetermineCodeMemoryCapability(heap_addr);
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
                        static_cast<u64>(self_proc_handle),
                        0
                    }
                },
                {
                    EntryKind::AppletType,
                    static_cast<u32>(SelfIsApplication() ? EntryAppletFlags::ApplicationOverride : EntryAppletFlags::None), {
                        static_cast<u64>(GetSelfAppletType()),
                        0
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
                        hosversionIsAtmosphere() ? 0x41544D4F53504852ul : 0 // 'ATMOSPHR'
                    }
                },
                {
                    EntryKind::EndOfList,
                    static_cast<u32>(EntryFlags::None), {
                        reinterpret_cast<u64>(g_NoticeText),
                        sizeof(g_NoticeText)
                    }
                }
            };
            static_assert(sizeof(g_TargetConfigEntries) == sizeof(target_cfg_entries));
            memcpy(g_TargetConfigEntries, target_cfg_entries, sizeof(target_cfg_entries));

            g_TargetMapAddress = map_addr;

            auto entrypoint = reinterpret_cast<Result(*)(ConfigEntry*, s64)>(g_TargetMapAddress);
            g_LastTargetResult = entrypoint(g_TargetConfigEntries, -1);

            return ResultSuccess;
        }
    }

    Result Target(const TargetInput &target_ipt, const bool is_auto_gameplay_recording, const u64 applet_heap_size, const u64 applet_heap_reservation_size) {
        BackupTls();

        u8 *heap_addr;
        u64 heap_size;
        UL_RC_TRY(SetupTargetHeap(is_auto_gameplay_recording, applet_heap_size, applet_heap_reservation_size, heap_addr, heap_size));

        do {
            UL_RC_TRY(DoTarget(target_ipt, heap_addr, heap_size));
        } while(!target_ipt.target_once);

        return ResultSuccess;
    }

}