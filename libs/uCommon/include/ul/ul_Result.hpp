
#pragma once
#include <ul/ul_Include.hpp>

#ifdef ATMOSPHERE
#include <stratosphere.hpp>
#endif

#include <ul/ul_Results.gen.hpp>

namespace ul {

    using namespace rc;
    using namespace rc::ulaunch;

    namespace res {

        template<typename T>
        inline ::Result TransformIntoResult(const T t) {
            return static_cast<::Result>(t);
        }

        #ifdef ATMOSPHERE
        template<>
        inline ::Result TransformIntoResult<::ams::Result>(const ::ams::Result ams_rc) {
            return ams_rc.GetValue();
        }
        #endif

    }

    #define UL_RC_TRY(res_expr) ({ \
        const auto _tmp_rc = ::ul::res::TransformIntoResult(res_expr); \
        if (R_FAILED(_tmp_rc)) { \
            return _tmp_rc; \
        } \
    })

    enum class LogKind {
        Information,
        Warning,
        Critical
    };

    void InitializeLogging(const char *proc_name);
    void LogImpl(const LogKind kind, const char *log_fmt, ...);
    void NX_NORETURN AbortImpl(const Result rc);

    #define UL_LOG_INFO(log_fmt, ...) ::ul::LogImpl(::ul::LogKind::Information, log_fmt, ##__VA_ARGS__)

    #define UL_LOG_WARN(log_fmt, ...) ::ul::LogImpl(::ul::LogKind::Warning, log_fmt, ##__VA_ARGS__)

    template<typename ...Args>
    inline void NX_NORETURN OnAssertionFailed(const Result rc, const char *log_fmt, Args &&...args) {
        LogImpl(LogKind::Critical, log_fmt, args...);

        AbortImpl(rc);
        __builtin_unreachable();
    }

    #define UL_RC_LOG_ASSERT(log, expr) ({ \
        const auto _tmp_rc = ::ul::res::TransformIntoResult(expr); \
        if(R_FAILED(_tmp_rc)) { \
            const char *mod_name; \
            const char *rc_name; \
            if(rc::GetResultNameAny(_tmp_rc, mod_name, rc_name)) { \
                ::ul::OnAssertionFailed(_tmp_rc, log " asserted %04d-%04d/0x%X/%s::%s...", R_MODULE(_tmp_rc) + 2000, R_DESCRIPTION(_tmp_rc), R_VALUE(_tmp_rc), mod_name, rc_name); \
            } \
            else { \
                ::ul::OnAssertionFailed(_tmp_rc, log " asserted %04d-%04d/0x%X...", R_MODULE(_tmp_rc) + 2000, R_DESCRIPTION(_tmp_rc), R_VALUE(_tmp_rc)); \
            } \
        } \
    })
    
    #define UL_RC_ASSERT(expr) UL_RC_LOG_ASSERT(#expr, expr)

    #define UL_ASSERT_TRUE(expr) ({ \
        const auto _tmp_expr = (expr); \
        if(!_tmp_expr) { \
            ::ul::OnAssertionFailed(::rc::ulaunch::ResultAssertionFailed, #expr " asserted to be false..."); \
        } \
    })

}
