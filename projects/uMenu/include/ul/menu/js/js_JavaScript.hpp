
#pragma once
#include <switch.h>
#include <tuple>
#include <functional>
#include <string>
#include <vector>

extern "C" {

    #include <math.h>
    #include <ul/menu/js/quickjs/quickjs.h>
    #include <ul/menu/js/quickjs/cutils.h>
    #include <ul/menu/js/quickjs/quickjs-libc.h>

    /*
    Manual changes made to QuickJS libraries:
    - Stubbed several "standard" components which we won't make use of anyway: dlfcn, termios, some libc stuff... (though this isn't an actual change, it's just adding new extra files, not modifying actual QuickJS code)
    - Also include <malloc.h> and adjust malloc_usable_size type in quickjs.c (QJS only did so under Linux)
    - Stubbed "getTimezoneOffset" in quickjs.c due to struct tm not containing a certain property needed there
    The rest is untouched, grabbed from https://bellard.org/quickjs/
    */

    // Custom version of some QJS macros, since those won't work on C++

    #define UL_MENU_JS_CFUNC_DEF(f_name, length, func1) JSCFunctionListEntry { .name = f_name, .prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CFUNC, .magic = 0, .u = { .func = { length, JS_CFUNC_generic, { .generic = func1 } } } }
    #define UL_MENU_JS_CGETSET_DEF(gs_name, fgetter, fsetter) JSCFunctionListEntry { .name = gs_name, .prop_flags = JS_PROP_CONFIGURABLE, .def_type = JS_DEF_CGETSET, .magic = 0, .u = { .getset = { .get = { .getter = fgetter }, .set = { .setter = fsetter } } } }
    #define UL_MENU_JS_PROP_STRING_DEF(p_name, cstr, p_flags) JSCFunctionListEntry { .name = p_name, .prop_flags = p_flags, .def_type = JS_DEF_PROP_STRING, .magic = 0, .u = { .str = cstr } }
    #define UL_MENU_JS_PROP_DOUBLE_DEF(p_name, val, p_flags) JSCFunctionListEntry { .name = p_name, .prop_flags = p_flags, .def_type = JS_DEF_PROP_DOUBLE, .magic = 0, .u = { .f64 = val } }

}

namespace ul::menu::js {

    constexpr auto Undefined = JS_UNDEFINED;
    constexpr auto Null = JS_NULL;

    struct Buffer {
        void *buf;
        size_t size;

        inline constexpr Buffer From(void *buf, size_t size) {
            return {
                .buf = buf,
                .size = size
            };
        }
    };

    class Reference {
        private:
            JSContext *js_ctx;
            JSValue orig_ref_object;
        
        public:
            Reference() {}

            Reference(JSContext *ctx, JSValue val) : js_ctx(ctx), orig_ref_object(val) {}

            template<typename T>
            void Set(T val);

            template<typename T>
            T Get();
    };

    template<typename T>
    concept ObjectType = requires(T, JSContext *ctx, JSValue new_target, int argc, JSValueConst *argv, JSRuntime *rt, JSValue val) {
        { T::GetJsClassId() } -> std::same_as<JSClassID>;
        { T::JsClassDef } -> std::convertible_to<const JSClassDef>;
        { T::JsCtor(ctx, new_target, argc, argv) } -> std::same_as<JSValue>;
        { T::JsDtor(rt, val) } -> std::same_as<void>;
    };

    template<ObjectType T>
    struct Object {
        JSValue value;
        
        Object() {}

        template<typename ...Ts>
        Object(Ts ...ts);

        Object(T *t_ptr);
        
        Object(JSValue val) : value(val) {}

        T *operator->() {
            return reinterpret_cast<T*>(JS_GetOpaque(this->value, T::GetJsClassId()));
        }

        static inline Object<T> Undefined() {
            return { js::Undefined };
        }

        static inline Object<T> Null() {
            return { js::Null };
        }
    };

    struct GenericObject {
        JSValue value;
        
        GenericObject() {}

        template<ObjectType T>
        GenericObject(Object<T> t_obj) : value(t_obj.value) {}
        
        GenericObject(JSValue val) : value(val) {}

        template<ObjectType T>
        T *Get() {
            return reinterpret_cast<T*>(JS_GetOpaque(this->value, T::GetJsClassId()));
        }

        static inline GenericObject Undefined() {
            return { js::Undefined };
        }

        static inline GenericObject Null() {
            return { js::Null };
        }
    };

    struct FunctionObject {
        JSValue value;

        FunctionObject() {}

        template<ObjectType T>
        FunctionObject(Object<T> t_obj) : value(t_obj.value) {}

        FunctionObject(GenericObject obj) : value(obj.value) {}
        
        FunctionObject(JSValue val) : value(val) {}

        template<typename Ret, typename ...Ts>
        Ret Call(Ts ...ts);
    };

    struct ModuleExport {
        const char *name;
        bool is_class;
        JSValue val;
        struct {
            JSClassID class_id;
            const JSClassDef *class_def;
            const JSCFunctionListEntry *class_prototype;
            const size_t class_prototype_size;
            JSCFunction *class_ctor;
        } class_data;

        static inline const ModuleExport MakeNonClass(const char *name, JSValue val) {
            return {
                .name = name,
                .is_class = false,
                .val = val
            };
        }

        static inline const ModuleExport MakeClass(const char *name, JSClassID class_id, const JSClassDef *class_def, const JSCFunctionListEntry *class_prototype, const size_t class_prototype_size, JSCFunction *class_ctor) {
            return {
                .name = name,
                .is_class = true,
                .class_data = {
                    .class_id = class_id,
                    .class_def = class_def,
                    .class_prototype = class_prototype,
                    .class_prototype_size = class_prototype_size,
                    .class_ctor = class_ctor
                }
            };
        }
    };

    struct NativeFunction {
        const char *fn_name;
        JSCFunction *fn;
        size_t arg_count;
    };

    namespace impl {

        JSContext *GetContextImpl();
        JSValue EvaluateImpl(const std::string &js_src, const std::string &eval_name, int mode = JS_EVAL_TYPE_GLOBAL);
        JSValue EvaluateFromFileImpl(const std::string &path, int mode = JS_EVAL_TYPE_GLOBAL);

        inline JSClassID NewClassIdImpl() {
            JSClassID class_id = 0;
            JS_NewClassID(&class_id);
            return class_id;
        }

        template<typename C>
        inline JSValue NewObjectFromImpl(JSContext *ctx, JSClassID id, C *instance_ptr) {
            auto instance = JS_NewObjectClass(ctx, id);
            JS_SetOpaque(instance, reinterpret_cast<void*>(instance_ptr));
            return instance;
        }

        template<typename T>
        struct IsObject {
            static constexpr bool Value = false;
        };

        template<typename U>
        struct IsObject<Object<U>> {
            static constexpr bool Value = true;
        };

        template<typename T>
        constexpr bool IsObjectValue = IsObject<std::decay_t<T>>::Value || std::is_same_v<std::decay_t<T>, GenericObject> || std::is_same_v<std::decay_t<T>, FunctionObject>;

        template<typename T>
        inline T UnpackImpl(JSContext *ctx, JSValue val) {
            if constexpr(std::is_same_v<T, const char*> || std::is_same_v<std::decay_t<T>, std::string>) {
                return JS_ToCString(ctx, val);
            }
            else if constexpr(std::is_same_v<std::decay_t<T>, bool>) {
                return static_cast<bool>(JS_ToBool(ctx, val));
            }
            else if constexpr(std::is_arithmetic_v<std::decay_t<T>>) {
                double ret = 0.0f;
                JS_ToFloat64(ctx, &ret, val);
                return static_cast<T>(ret);
            }
            else if constexpr(std::is_same_v<std::decay_t<T>, Buffer>) {
                js::Buffer buf = {};
                buf.buf = JS_GetArrayBuffer(ctx, &buf.size, val);
                return buf;
            }
            else if constexpr(std::is_same_v<std::decay_t<T>, Reference>) {
                return T(ctx, val);
            }
            // NativeFunction is unsupported here for obvious reasons
            else if constexpr(IsObjectValue<std::decay_t<T>>) {
                return { val };
            }
            else if constexpr(std::is_same_v<std::decay_t<T>, JSValue>) {
                return val;
            }
            return T();
        }

        template<typename T>
        inline JSValue PackImpl(JSContext *ctx, T t) {
            if constexpr(std::is_same_v<T, const char*>) {
                return JS_NewString(ctx, t);
            }
            else if constexpr(std::is_same_v<std::decay_t<T>, std::string>) {
                return JS_NewString(ctx, t.c_str());
            }
            else if constexpr(std::is_same_v<std::decay_t<T>, bool>) {
                return JS_NewBool(ctx, t);
            }
            else if constexpr(std::is_arithmetic_v<std::decay_t<T>>) {
                return JS_NewFloat64(ctx, static_cast<double>(t));
            }
            else if constexpr(std::is_same_v<std::decay_t<T>, Buffer>) {
                return JS_NewArrayBufferCopy(ctx, const_cast<const u8*>(t.buf), t.size);
            }
            // Reference is unsupported here for obvious reasons
            else if constexpr(std::is_same_v<std::decay_t<T>, NativeFunction>) {
                return JS_NewCFunction(ctx, t.fn, t.fn_name, t.arg_count);
            }
            else if constexpr(IsObjectValue<std::decay_t<T>>) {
                return t.value;
            }
            else if constexpr(std::is_same_v<std::decay_t<T>, JSValue>) {
                return t;
            }
            return Undefined;
        }

        class FunctionHelper {
            private:
                JSContext *fn_ctx;
                JSValueConst fn_this;
                int fn_argc;
                JSValueConst *fn_argv;
                int h_argc;
            
            public:
                FunctionHelper(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) : fn_ctx(ctx), fn_this(this_val), fn_argc(argc), fn_argv(argv), h_argc(0) {}

                inline JSContext *GetContext() {
                    return this->fn_ctx;
                }

                JSValueConst PopArgv() {
                    auto val = this->fn_argv[this->h_argc];
                    this->h_argc++;
                    return val;
                }

                inline constexpr bool CanPopArgv() {
                    return this->h_argc < this->fn_argc;
                }
        };

        template<typename T, typename ...Ts>
        struct ProcessValues {
            typedef typename std::conditional<sizeof...(Ts) == 0, std::tuple<T>, std::tuple<T, Ts...>>::type TupleValue;

            static void Impl(FunctionHelper &helper, TupleValue &tpl) {
                T param = T();
                if(helper.CanPopArgv()) {
                    param = UnpackImpl<T>(helper.GetContext(), helper.PopArgv());
                }
                else {
                    // TODO: errors?
                }

                std::tuple<T> single = std::make_tuple(param);

                if constexpr(sizeof...(Ts) > 0) {
                    std::tuple<Ts...> pack;
                    ProcessValues<Ts...>::Impl(helper, pack);
                    tpl = std::tuple_cat(single, pack);
                }
                else {
                    tpl = single;
                }
            }
        };

        template<typename ...Ts>
        std::tuple<Ts...> ProcessParamsImpl(FunctionHelper &helper) {
            std::tuple<Ts...> tpl;
            ProcessValues<Ts...>::Impl(helper, tpl);
            return tpl;
        }

        template<typename T>
        struct ParameterCount {
            static constexpr size_t Value = 0;
        };

        template<typename Ret, typename ...Ts>
        struct ParameterCount<Ret(*)(Ts...)> {
            static constexpr size_t Value = sizeof...(Ts);
        };

        template<typename C, typename Ret, typename ...Ts>
        struct ParameterCount<Ret(C::*)(Ts...)> {
            static constexpr size_t Value = sizeof...(Ts);
        };

        template<typename T>
        constexpr size_t ParameterCountValue = ParameterCount<T>::Value;

        template<typename T>
        struct OnlyParameter {
            using Type = void;
        };

        template<typename Ret, typename T>
        struct OnlyParameter<Ret(*)(T)> {
            using Type = T;
        };

        template<typename Ret, typename C, typename T>
        struct OnlyParameter<Ret(C::*)(T)> {
            using Type = T;
        };

        template<typename T>
        struct CallNativeFn {
            static void Impl(FunctionHelper &helper, T t) {}
        };

        template<typename ...Ts>
        struct CallNativeFn<void(*)(Ts...)> {
            static JSValue Impl(FunctionHelper &helper, void(*fn)(Ts...)) {
                if constexpr(sizeof...(Ts) == 0) {
                    fn();
                }
                else {
                    auto params = ProcessParamsImpl<Ts...>(helper);
                    std::apply(fn, params);
                }
                return Undefined;
            }
        };
        
        template<typename Ret, typename ...Ts>
        struct CallNativeFn<Ret(*)(Ts...)> {
            static Ret Impl(FunctionHelper &helper, Ret(*fn)(Ts...)) {
                if constexpr(sizeof...(Ts) == 0) {
                    auto res = fn();
                    return res;
                }
                else {
                    auto params = ProcessParamsImpl<Ts...>(helper);
                    auto res = std::apply(fn, params);
                    return res;
                }
                return Ret();
            }
        };

        template<typename T>
        struct CallMemberFn {
            static void Impl(FunctionHelper &helper, T t) {}
        };

        template<ObjectType C, typename ...Ts>
        struct CallMemberFn<void(C::*)(Ts...)> {
            static JSValue Impl(FunctionHelper &helper, C *this_ptr, void(C::*fn)(Ts...)) {
                if constexpr(sizeof...(Ts) == 0) {
                    (this_ptr->*fn)();
                }
                else {
                    std::tuple<C*> this_tpl = { this_ptr };
                    auto params = ProcessParamsImpl<Ts...>(helper);
                    std::apply(fn, std::tuple_cat(this_tpl, params));
                }
                return Undefined;
            }
        };

        template<typename Ret, typename C, typename ...Ts>
        struct CallMemberFn<Ret(C::*)(Ts...)> {
            static Ret Impl(FunctionHelper &helper, C *this_ptr, Ret(C::*fn)(Ts...)) {
                if constexpr(sizeof...(Ts) == 0) {
                    auto res = (this_ptr->*fn)();
                    return res;
                }
                else {
                    std::tuple<C*> this_tpl = { this_ptr };
                    auto params = ProcessParamsImpl<Ts...>(helper);
                    auto res = std::apply(fn, std::tuple_cat(this_tpl, params));
                    return res;
                }
                return Ret();
            }
        };

        template<typename T, typename Tpl, size_t ...Idx>
        constexpr T *NewFromTupleImpl(Tpl &&__t, std::index_sequence<Idx...>) {
            return new T(std::get<Idx>(std::forward<Tpl>(__t))...);
        }

        template<typename T, typename Tpl>
        constexpr T *NewFromTuple(Tpl &&__t) {
            return NewFromTupleImpl<T>(std::forward<Tpl>(__t), std::make_index_sequence<std::tuple_size_v<std::decay_t<Tpl>>>{});
        }

        template<ObjectType C, typename ...Ts>
        inline C *CallClassCtorImpl(FunctionHelper &helper) {
            if constexpr(sizeof...(Ts) == 0) {
                return new C();
            }
            else {
                auto params = ProcessParamsImpl<Ts...>(helper);
                return NewFromTuple<C>(params);
            }
            return nullptr;
        }

        template<typename T, typename ...Ts>
        struct ProcessPackMulti {
            static void Impl(JSContext *ctx, JSValue *&values, size_t &idx, T t, Ts ...ts) {
                values[idx] = PackImpl(ctx, t);
                idx++;
                if constexpr(sizeof...(Ts) > 0) {
                    ProcessPackMulti<Ts...>::Impl(ctx, values, idx, ts...);
                }
            }
        };

        template<typename ...Ts>
        inline JSValue *PackMultiImpl(JSContext *ctx, Ts ...ts) {
            auto values = new JSValue[sizeof...(Ts)]();
            size_t idx = 0;

            if constexpr(sizeof...(Ts) > 0) {
                ProcessPackMulti<Ts...>::Impl(ctx, values, idx, ts...);
            }

            return values;
        }

        void PushGlobalFunctionImpl(JSContext *ctx, JSCFunction fn, size_t arg_count, const std::string &name);

        template<typename T>
        inline JSCFunctionListEntry MakePropertyImpl(const std::string &name, T val, int flags) {
            if constexpr(std::is_same<std::decay_t<T>, std::string>::value) {
                return UL_MENU_JS_PROP_STRING_DEF(name.c_str(), val.c_str(), flags);
            }
            else if constexpr(std::is_same<T, const char*>::value) {
                return UL_MENU_JS_PROP_STRING_DEF(name.c_str(), val, flags);
            }
            else if constexpr(std::is_arithmetic<std::decay_t<T>>::value) {
                return UL_MENU_JS_PROP_DOUBLE_DEF(name.c_str(), static_cast<double>(val), flags);
            }
            return {};
        }

        void ImportModuleImpl(const std::string &name);
    }

    template<typename T>
    void Reference::Set(T val) {
        JS_SetPropertyStr(this->js_ctx, this->orig_ref_object, "v", impl::PackImpl<T>(this->js_ctx, val));
    }

    template<typename T>
    T Reference::Get() {
        return impl::UnpackImpl<T>(this->js_ctx, JS_GetPropertyStr(this->js_ctx, this->orig_ref_object, "v"));
    }

    template<typename T>
    template<typename ...Ts>
    Object<T>::Object(Ts ...ts) : value(impl::NewObjectFromImpl(impl::GetContextImpl(), T::GetJsClassId(), new T(ts...))) {}

    template<typename T>
    Object<T>::Object(T *t_ptr) : value(impl::NewObjectFromImpl(impl::GetContextImpl(), T::GetJsClassId(), t_ptr)) {}

    template<typename Ret, typename ...Ts>
    Ret FunctionObject::Call(Ts ...ts) {
        auto ctx = impl::GetContextImpl();
        const auto values = impl::PackMultiImpl(ctx, ts...);
        
        auto res = JS_Call(ctx, this->value, Undefined, sizeof...(Ts), values);
        delete[] values;
        return impl::UnpackImpl<Ret>(ctx, res);
    }

    void Initialize();
    bool IsInitialized();
    void Finalize();

    void ImportBuiltinStdModule(const std::string &mod_name = "std");
    void ImportBuiltinOsModule(const std::string &mod_name = "os");

    template<typename T>
    inline T Evaluate(const std::string &js_src, const std::string &eval_name, int mode = JS_EVAL_TYPE_GLOBAL) {
        return impl::UnpackImpl<T>(impl::GetContextImpl(), impl::EvaluateImpl(js_src, eval_name, mode));
    }

    template<typename T>
    inline T EvaluateFromFile(const std::string &path, int mode = JS_EVAL_TYPE_GLOBAL) {
        return impl::UnpackImpl<T>(impl::GetContextImpl(), impl::EvaluateFromFileImpl(path, mode));
    }

    std::string EvaluateEx(const std::string &js_src, const std::string &eval_name, int mode = JS_EVAL_TYPE_GLOBAL);
    std::string EvaluateFromFileEx(const std::string &path, int mode = JS_EVAL_TYPE_GLOBAL);

    std::string RetrieveExceptionStacktrace();

    // TODO: custom ToString?

    inline std::string ValueToString(JSValue val) {
        return JS_ToCString(impl::GetContextImpl(), val);
    }

    template<typename T>
    inline void SetObjectProperty(JSValue this_obj, const std::string &name, T val) {
        JS_SetPropertyStr(impl::GetContextImpl(), this_obj, name.c_str(), impl::PackImpl(impl::GetContextImpl(), val));
    }

    template<typename T>
    inline T GetObjectProperty(JSValue this_obj, const std::string &name) {
        return impl::UnpackImpl<T>(impl::GetContextImpl(), JS_GetPropertyStr(impl::GetContextImpl(), this_obj, name.c_str()));
    }

    template<typename T>
    inline void SetGlobalProperty(const std::string &name, T val) {
        auto global_obj = JS_GetGlobalObject(impl::GetContextImpl());
        SetObjectProperty(global_obj, name, impl::PackImpl(impl::GetContextImpl(), val));
    }

    #define UL_MENU_JS_DEFINE_FUNCTION(name) \
    static JSValue __ul_menu_js_fn_##name(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { \
        ::ul::menu::js::impl::FunctionHelper helper(ctx, this_val, argc, argv); \
        auto native_fn = &name; \
        auto ret = ::ul::menu::js::impl::CallNativeFn<decltype(native_fn)>::Impl(helper, native_fn); \
        return ::ul::menu::js::impl::PackImpl(ctx, ret); \
    }

    #define UL_MENU_JS_CLASS_DECLARE(name, ...) \
    static JSClassID GetJsClassId() { \
        static JSClassID class_id = ::ul::menu::js::impl::NewClassIdImpl(); \
        return class_id; \
    } \
    using Self = name; \
    static JSValue JsCtor(JSContext *ctx, JSValue new_target, int argc, JSValueConst *argv) { \
        ::ul::menu::js::impl::FunctionHelper helper(ctx, new_target, argc, argv); \
        auto this_ptr = ::ul::menu::js::impl::CallClassCtorImpl<name, ##__VA_ARGS__>(helper); \
        auto prototype = JS_GetPropertyStr(ctx, new_target, "prototype"); \
        auto instance = JS_NewObjectProtoClass(ctx, prototype, GetJsClassId()); \
        JS_FreeValue(ctx, prototype); \
        JS_SetOpaque(instance, reinterpret_cast<void*>(this_ptr)); \
        return instance; \
    } \
    static void JsDtor(JSRuntime *rt, JSValue val) { \
        auto this_ptr = reinterpret_cast<name*>(JS_GetOpaque(val, GetJsClassId())); \
        delete this_ptr; \
    } \
    static constexpr JSClassDef JsClassDef = { \
        .class_name = #name, \
        .finalizer = JsDtor, \
        .gc_mark = nullptr, \
        .call = nullptr, \
        .exotic = nullptr, \
    };

    #define _UL_MENU_JS_MEMBER_GETTER(name) name##_get
    #define _UL_MENU_JS_MEMBER_SETTER(name) name##_set

    #define UL_MENU_JS_CLASS_DECLARE_PROPERTY_G(prop_name) \
    static JSValue JsGetter_##prop_name(JSContext *ctx, JSValueConst this_val) { \
        auto this_ptr = reinterpret_cast<Self*>(JS_GetOpaque2(ctx, this_val, GetJsClassId())); \
        auto res = this_ptr->_UL_MENU_JS_MEMBER_GETTER(prop_name)(); \
        auto val = ::ul::menu::js::impl::PackImpl(ctx, res); \
        return val; \
    }

    #define UL_MENU_JS_CLASS_DECLARE_PROPERTY_S(prop_name) \
    static JSValue JsSetter_##prop_name(JSContext *ctx, JSValueConst this_val, JSValue val) { \
        auto this_ptr = reinterpret_cast<Self*>(JS_GetOpaque2(ctx, this_val, GetJsClassId())); \
        auto res = ::ul::menu::js::impl::UnpackImpl<::ul::menu::js::impl::OnlyParameter<decltype(&Self::_UL_MENU_JS_MEMBER_SETTER(prop_name))>::Type>(ctx, val); \
        this_ptr->_UL_MENU_JS_MEMBER_SETTER(prop_name)(res); \
        return ::ul::menu::js::Undefined; \
    }

    #define UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(prop_name) \
    UL_MENU_JS_CLASS_DECLARE_PROPERTY_G(prop_name) \
    UL_MENU_JS_CLASS_DECLARE_PROPERTY_S(prop_name)

    #define UL_MENU_JS_CLASS_DECLARE_MEMBER_FN(fn_name) \
    static JSValue JsMemberFn_##fn_name(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { \
        auto this_ptr = reinterpret_cast<Self*>(JS_GetOpaque2(ctx, this_val, GetJsClassId())); \
        ::ul::menu::js::impl::FunctionHelper helper(ctx, this_val, argc, argv); \
        auto member_fn = &Self::fn_name; \
        auto ret = ::ul::menu::js::impl::CallMemberFn<decltype(member_fn)>::Impl(helper, this_ptr, member_fn); \
        return ::ul::menu::js::impl::PackImpl(ctx, ret); \
    }

    #define UL_MENU_JS_CLASS_PROTOTYPE_START static constexpr JSCFunctionListEntry JsPrototype[] = { \

    #define UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(prop_name) UL_MENU_JS_CGETSET_DEF(#prop_name, JsGetter_##prop_name, JsSetter_##prop_name),
    #define UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_G(prop_name) UL_MENU_JS_CGETSET_DEF(#prop_name, JsGetter_##prop_name, nullptr),
    #define UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_S(prop_name) UL_MENU_JS_CGETSET_DEF(#prop_name, nullptr, JsSetter_##prop_name),
    #define UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_MEMBER_FN(fn_name) UL_MENU_JS_CFUNC_DEF(#fn_name, ::ul::menu::js::impl::ParameterCountValue<decltype(&Self::fn_name)>, JsMemberFn_##fn_name),

    #define UL_MENU_JS_CLASS_PROTOTYPE_END };

    // Global (todo)

    // Module

    #define UL_MENU_JS_MODULE_DEFINE_EXPORTS_START(mod_name) static const std::vector<::ul::menu::js::ModuleExport> __ul_menu_js_module_##mod_name##_GetExports(JSContext *ctx) { \
        static const std::vector<::ul::menu::js::ModuleExport> exports = {

    #define UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_PROPERTY(name) ::ul::menu::js::ModuleExport::MakeNonClass(#name, ::ul::menu::js::impl::PackImpl(ctx, name)),

    #define UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_CLASS(name) ::ul::menu::js::ModuleExport::MakeClass(#name, name::GetJsClassId(), &name::JsClassDef, name::JsPrototype, countof(name::JsPrototype), name::JsCtor),

    #define UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(name) ::ul::menu::js::ModuleExport::MakeNonClass(#name, ::ul::menu::js::impl::PackImpl(ctx, ::ul::menu::js::NativeFunction { #name, __ul_menu_js_fn_##name, ::ul::menu::js::impl::ParameterCountValue<decltype(&name)> })),

    #define UL_MENU_JS_MODULE_DEFINE_EXPORTS_END }; return exports; }
    
    // TODO: figure out ctor parameter count?

    #define UL_MENU_JS_IMPORT_MODULE(mod_name) { \
        struct ModuleInit_##mod_name { \
            static int DoInit(JSContext *ctx, JSModuleDef *mod) { \
                for(const auto &mod_export : __ul_menu_js_module_##mod_name##_GetExports(ctx)) { \
                    if(mod_export.is_class) { \
                        JS_NewClass(JS_GetRuntime(ctx), mod_export.class_data.class_id, mod_export.class_data.class_def); \
                        auto prototype = JS_NewObject(ctx); \
                        JS_SetPropertyFunctionList(ctx, prototype, mod_export.class_data.class_prototype, mod_export.class_data.class_prototype_size); \
                        JS_SetClassProto(ctx, mod_export.class_data.class_id, prototype); \
                        auto ctor_fn = JS_NewCFunction2(ctx, mod_export.class_data.class_ctor, mod_export.name, 0, JS_CFUNC_constructor, 0); \
                        JS_SetConstructor(ctx, ctor_fn, prototype); \
                        JS_SetModuleExport(ctx, mod, mod_export.name, ctor_fn); \
                    } \
                    else { \
                        JS_SetModuleExport(ctx, mod, mod_export.name, mod_export.val); \
                    } \
                } \
                return 0; \
            } \
        }; \
        auto ctx = ::ul::menu::js::impl::GetContextImpl(); \
        auto mod = JS_NewCModule(ctx, #mod_name, ModuleInit_##mod_name::DoInit); \
        if(mod != nullptr) { \
            for(const auto &mod_export : __ul_menu_js_module_##mod_name##_GetExports(ctx)) { \
                JS_AddModuleExport(ctx, mod, mod_export.name); \
            } \
            ::ul::menu::js::impl::ImportModuleImpl(#mod_name); \
        } \
    }

}