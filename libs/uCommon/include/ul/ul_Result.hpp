
#pragma once
#include <switch.h>
#include <cstdio>

#ifdef ATMOSPHERE
#include <stratosphere.hpp>
#endif

namespace ul {

    // All 2380-**** results are from us

    constexpr u32 Module = 380;
    constexpr u32 SubmoduleOffset = 100;

    #define UL_RC_DEFINE_SUBMODULE(val) constexpr u32 Submodule = val
    #define UL_RC_DEFINE(name, val) constexpr Result Result ## name = MAKERESULT(Module, Submodule * SubmoduleOffset + val)

    constexpr Result ResultSuccess = 0;

    /*
    
    Result submodules:
    0 -> misc
    1 -> smi
    2 -> sf (ipc)
    3 -> loader
    4 -> smi
    5 -> util
    6 -> uMenu
    
    */

    namespace res {

        UL_RC_DEFINE_SUBMODULE(0);

        UL_RC_DEFINE(AssertionFailed, 1);
        UL_RC_DEFINE(InvalidTransform, 2);

        template<typename T>
        inline ::Result TransformIntoResult(const T t) {
            return static_cast<::Result>(t);
        }

        #ifdef ATMOSPHERE
        template<>
        inline ::Result TransformIntoResult<ams::Result>(const ams::Result ams_rc) {
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

    constexpr auto AssertionLogFile = "sdmc:/umad/assert.log";

    template<typename ...Args>
    inline void NORETURN OnAssertionFailed(const Result rc, const char *log_fmt, Args &&...args) {
        // TODO: unique log file for each assertion fatal?
        auto log_f = fopen(AssertionLogFile, "wb");
        if(log_f) {
            fprintf(log_f, log_fmt, args...);
            fclose(log_f);
        }

        svcBreak(0, reinterpret_cast<uintptr_t>(&rc), sizeof(rc));
        __builtin_unreachable();
    }

    #define UL_RC_ASSERT(expr) ({ \
        const auto _tmp_rc = ::ul::res::TransformIntoResult(expr); \
        if(R_FAILED(_tmp_rc)) { \
            ::ul::OnAssertionFailed(_tmp_rc, #expr " asserted 0x%X...", _tmp_rc); \
        } \
    })

    #define UL_ASSERT_TRUE(expr) ({ \
        const auto _tmp_expr = (expr); \
        if(!_tmp_expr) { \
            ::ul::OnAssertionFailed(::ul::res::ResultAssertionFailed, #expr " asserted to be false..."); \
        } \
    })


}