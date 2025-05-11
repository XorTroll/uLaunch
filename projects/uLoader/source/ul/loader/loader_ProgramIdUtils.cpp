#include <ul/loader/loader_ProgramIdUtils.hpp>
#include <ul/ul_Result.hpp>

extern "C" {

    u32 __nx_applet_type;

}

namespace ul::loader {

    void DetermineSelfAppletType(const u64 self_program_id) {
        __nx_applet_type = GetAppletType(self_program_id);
        UL_LOG_INFO("Program ID: 0x%016lX --> applet type: %d", self_program_id, __nx_applet_type);
    }

    AppletType GetSelfAppletType() {
        return static_cast<AppletType>(__nx_applet_type);
    }

    bool SelfIsApplet() {
        return __nx_applet_type == Applet;
    }

    bool SelfIsApplication() {
        return __nx_applet_type == Application;
    }

}
