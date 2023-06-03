#include <ul/menu/js/js_JavaScript.hpp>
#include <cstdio>

namespace ul::menu::js {

    namespace impl {

        namespace {

            JSRuntime *g_Runtime = nullptr;
            JSContext *g_Context = nullptr;
            bool g_Initialized = false;

        }

        JSContext *GetContextImpl() {
            return g_Context;
        }

        std::string RetrieveExceptionStacktraceImpl(JSContext *ctx) {
            std::string stacktrace;
            auto exception_val = JS_GetException(ctx);
            auto is_error = static_cast<bool>(JS_IsError(ctx, exception_val));
            if(!is_error) {
                stacktrace += "Throw: ";
            }
            stacktrace = JS_ToCString(ctx, exception_val);
            if(is_error) {
                auto val = JS_GetPropertyStr(ctx, exception_val, "stack");
                if(!JS_IsUndefined(val)) {
                    auto stack = JS_ToCString(ctx, val);
                    stacktrace += stack;
                    stacktrace += "\n";
                    JS_FreeCString(ctx, stack);
                }
                JS_FreeValue(ctx, val);
            }
            JS_FreeValue(ctx, exception_val);
            return stacktrace;
        }

        JSValue EvaluateImpl(const std::string &js_src, const std::string &eval_name, int mode) {
            if(!impl::g_Initialized) {
                return Null;
            }
            return JS_Eval(impl::g_Context, js_src.c_str(), js_src.length(), eval_name.c_str(), mode);
        }

        JSValue EvaluateFromFileImpl(const std::string &path, int mode) {
            size_t file_sz = 0;
            auto file_buf = js_load_file(impl::g_Context, &file_sz, path.c_str());
            if(file_buf != nullptr) {
                if(file_sz > 0) {
                    auto res = EvaluateImpl(reinterpret_cast<const char*>(file_buf), path, mode);
                    js_free(impl::g_Context, file_buf);
                    return res;
                }
            }
            return Undefined;
        }

        void ImportModuleImpl(const std::string &name) {
            std::string ipt = "import * as " + name + " from '" + name + "';\n";
            ipt += "globalThis." + name + " = " + name + ";";
            /* auto res = */ EvaluateImpl(ipt, "<module-import>", JS_EVAL_TYPE_MODULE);
        }

    }

    void Initialize() {
        if(!impl::g_Initialized) {
            impl::g_Runtime = JS_NewRuntime();
            if(impl::g_Runtime != nullptr) {
                impl::g_Context = JS_NewContext(impl::g_Runtime);
                if(impl::g_Context != nullptr) {
                    JS_SetModuleLoaderFunc(impl::g_Runtime, nullptr, js_module_loader, nullptr);
                    impl::g_Initialized = true;
                }
            }
        }
    }

    bool IsInitialized() {
        return impl::g_Initialized;
    }

    void Finalize() {
        if(impl::g_Initialized) {
            if(impl::g_Context != nullptr) {
                JS_FreeContext(impl::g_Context);
                impl::g_Context = nullptr;
            }
            if(impl::g_Runtime != nullptr) {
                JS_FreeRuntime(impl::g_Runtime);
                impl::g_Runtime = nullptr;
            }
            impl::g_Initialized = false;
        }
    }

    void ImportBuiltinStdModule(const std::string &mod_name) {
        js_init_module_std(impl::g_Context, mod_name.c_str());
        impl::ImportModuleImpl(mod_name);
    }

    void ImportBuiltinOsModule(const std::string &mod_name) {
        js_init_module_os(impl::g_Context, mod_name.c_str());
        impl::ImportModuleImpl(mod_name);
    }

    std::string EvaluateEx(const std::string &js_src, const std::string &eval_name, int mode) {
        auto val = impl::EvaluateImpl(js_src, eval_name, mode);
        if(JS_IsException(val)) {
            return RetrieveExceptionStacktrace();
        }
        return JS_ToCString(impl::g_Context, val);
    }

    std::string EvaluateFromFileEx(const std::string &path, int mode) {
        auto val = impl::EvaluateFromFileImpl(path, mode);
        if(JS_IsException(val)) {
            return RetrieveExceptionStacktrace();
        }
        return JS_ToCString(impl::g_Context, val);
    }

    std::string RetrieveExceptionStacktrace() {
        return impl::RetrieveExceptionStacktraceImpl(impl::g_Context);
    }

}