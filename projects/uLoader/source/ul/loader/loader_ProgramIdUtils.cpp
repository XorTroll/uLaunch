#include <ul/loader/loader_ProgramIdUtils.hpp>

extern "C" {

    u32 __nx_applet_type;

}

namespace ul::loader {

    void DetermineSelfAppletType(const u64 self_program_id) {
        __nx_applet_type = GetAppletType(self_program_id);
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