#include <ul/loader/loader_SelfProcess.hpp>
#include <ul/loader/loader_Target.hpp>
#include <ul/loader/loader_Input.hpp>
#include <ul/loader/loader_ProgramIdUtils.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/util/util_Size.hpp>

using namespace ul::util::size;

namespace {

    constexpr auto HbloaderSettingsSectionName = "hbloader";

    template<typename T>
    inline Result GetHbloaderSetting(const char *key, T &out_value) {
        u64 setting_size;
        UL_RC_TRY(setsysGetSettingsItemValue(HbloaderSettingsSectionName, key, std::addressof(out_value), sizeof(out_value), &setting_size));

        if(setting_size != sizeof(out_value)) {
            return MAKERESULT(Module_Libnx, LibnxError_BadInput);
        }
        
        return ul::ResultSuccess;
    }

    NsApplicationControlData g_SelfControlData = {};

    Result ReadSelfControlData(const u64 self_program_id) {
        if(hosversionAtLeast(5,0,0)) {
            UL_RC_TRY(nsInitialize());
            ul::util::OnScopeExit exit_ns([]() {
                nsExit();
            });

            size_t tmp_size;
            UL_RC_TRY(nsGetApplicationControlData(NsApplicationControlSource_Storage, self_program_id, &g_SelfControlData, sizeof(g_SelfControlData), &tmp_size));
        }
        
        return ul::ResultSuccess;
    }

    constexpr size_t HeapSize = 32_KB;
    u8 g_Heap[HeapSize] = {};

}

extern "C" {

    u32 __nx_applet_exit_mode = 2;

    u32 __nx_fs_num_sessions = 1;
    bool __nx_fsdev_support_cwd = false;

    extern u8 *fake_heap_start;
    extern u8 *fake_heap_end;

    void __libnx_initheap() {
        fake_heap_start = g_Heap;
        fake_heap_end = g_Heap + HeapSize;
    }

    void __appInit() {}
    void __appExit() {}

}

int main() {
    ul::InitializeLogging("uLoader");

    UL_RC_ASSERT(smInitialize());

    UL_RC_ASSERT(fsInitialize());
    UL_RC_ASSERT(fsdevMountSdmc());

    UL_RC_ASSERT(setsysInitialize());
    
    SetSysFirmwareVersion fw_ver;
    UL_RC_ASSERT(setsysGetFirmwareVersion(&fw_ver));
    hosversionSet(MAKEHOSVERSION(fw_ver.major, fw_ver.minor, fw_ver.micro));

    u64 applet_heap_size;
    UL_RC_ASSERT(GetHbloaderSetting("applet_heap_size", applet_heap_size));
    u64 applet_heap_reservation_size;
    UL_RC_ASSERT(GetHbloaderSetting("applet_heap_reservation_size", applet_heap_reservation_size));

    setsysExit();

    u64 self_program_id;
    UL_RC_ASSERT(ul::loader::GetSelfProgramId(self_program_id));
    ul::loader::DetermineSelfAppletType(self_program_id);

    ul::loader::TargetInput target_ipt;
    UL_RC_ASSERT(ul::loader::ReadTargetInput(target_ipt));

    if(ul::loader::SelfIsApplication()) {
        // We really don't care about the result
        ReadSelfControlData(self_program_id);
    }
    const auto is_auto_game_recording = g_SelfControlData.nacp.video_capture == 2;

    UL_LOG_INFO("Targetting '%s' with argv '%s' (once: %d)", target_ipt.nro_path, target_ipt.nro_argv, target_ipt.target_once);

    fsdevUnmountAll();
    fsExit();
    smExit();

    UL_RC_ASSERT(ul::loader::Target(target_ipt, is_auto_game_recording, applet_heap_size, applet_heap_reservation_size));

    ul::loader::TargetOutput target_opt;
    ul::loader::LoadTargetOutput(target_opt);

    UL_LOG_INFO("Sending target output... '%s' with argv '%s'", target_opt.nro_path, target_opt.nro_argv);

    UL_RC_ASSERT(smInitialize());
    UL_RC_ASSERT(ul::loader::WriteTargetOutput(target_opt));
    smExit();

    return 0;
}