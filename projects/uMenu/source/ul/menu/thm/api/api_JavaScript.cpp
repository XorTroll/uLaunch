#include <ul/menu/thm/api/api_Api.hpp>

extern ul::menu::thm::Theme g_Theme;

namespace ul::menu::thm::api {

    namespace {

        inline bool EvaluateScriptImpl(const std::string &src) {
            api::DebugLog("Eval script impl");
            auto res = js::Evaluate<JSValue>(src, "<script-eval>");
            if(JS_IsException(res)) {
                api::DebugLog("Exception happened -> " + js::RetrieveExceptionStacktrace());
                return false;
            }
            else {
                api::DebugLog("Return -> " + js::ValueToString(res));
                return true;
            }
        }

    }

    bool EvaluateScript(const MenuId id) {
        api::DebugLog("Eval script");
        UpdateCurrentMenu(id);

        const auto src = g_Theme.LoadMenuSource(id);
        api::DebugLog("Eval script impl");
        const auto res = js::Evaluate<JSValue>(src, "<script-eval>");
        if(JS_IsException(res)) {
            api::DebugLog("Exception happened -> " + js::RetrieveExceptionStacktrace());
            return false;
        }
        else {
            api::DebugLog("Return -> " + js::ValueToString(res));
            return true;
        }

        return false;
    }

}