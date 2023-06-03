#include <ul/menu/thm/api/api_Api.hpp>

namespace ul::menu::thm::api::os {

    namespace {

        constexpr const char *DebugLogPath = "sdmc:/ulaunch/js_debug.log";

    }

    void debugLog(std::string log) {
        auto log_msg = log + "\n";
        fs::WriteFile(DebugLogPath, log_msg.data(), log_msg.length(), false);
    }
    UL_MENU_JS_DEFINE_FUNCTION(debugLog)

    UL_MENU_JS_MODULE_DEFINE_EXPORTS_START(os)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(debugLog)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_END

    void InitializeModule() {
        UL_MENU_JS_IMPORT_MODULE(os)
    }

}