
#pragma once
#include <switch.h>
#include <string>

#ifdef ATMOSPHERE
#include <stratosphere.hpp>
#endif

// All 2380-**** results are from us

constexpr u32 Module = 380;
constexpr u32 SubmoduleOffset = 100;

#define UL_RC_DEFINE_SUBMODULE(val) constexpr u32 Submodule = val
#define UL_RC_DEFINE(name, val) constexpr Result Result ## name = MAKERESULT(Module, Submodule * SubmoduleOffset + val)

constexpr Result ResultSuccess = 0;

namespace misc {

    UL_RC_DEFINE_SUBMODULE(0);
    UL_RC_DEFINE(AssertionFailed, 1);
    UL_RC_DEFINE(UnexpectedTransform, 2);
    UL_RC_DEFINE(InvalidJsonFile, 3);

}

namespace dmn {

    UL_RC_DEFINE_SUBMODULE(1);
    UL_RC_DEFINE(ApplicationActive, 1);
    UL_RC_DEFINE(InvalidSelectedUser, 2);
    UL_RC_DEFINE(AlreadyQueued, 3);
    UL_RC_DEFINE(ApplicationNotActive, 4);

}

namespace menu {

    UL_RC_DEFINE_SUBMODULE(2);
    UL_RC_DEFINE(RomfsFileNotFound, 1);

}

namespace ipc {

    UL_RC_DEFINE_SUBMODULE(3);
    UL_RC_DEFINE(InvalidProcess, 1);

}

namespace dmi {

    UL_RC_DEFINE_SUBMODULE(4);
    UL_RC_DEFINE(OutOfPushSpace, 1);
    UL_RC_DEFINE(OutOfPopSpace, 2);
    UL_RC_DEFINE(InvalidInHeaderMagic, 3);
    UL_RC_DEFINE(InvalidOutHeaderMagic, 4);
    UL_RC_DEFINE(WaitTimeout, 5);

}

namespace res {

    template<typename T>
    inline ::Result TransformIntoResult(const T t) {
        return misc::ResultUnexpectedTransform;
    }

    template<>
    inline ::Result TransformIntoResult<::Result>(const ::Result rc) {
        return rc;
    }

    #ifdef ATMOSPHERE
    template<>
    inline ::Result TransformIntoResult<ams::Result>(const ams::Result ams_rc) {
        return ams_rc.GetValue();
    }
    #endif

    Result GetResultByModuleAndName(const std::string &mod, const std::string &name);
    std::string GetModuleByResult(const Result rc);
    std::string GetNameByResult(const Result rc);

}

#define UL_RC_TRY(res_expr) ({ \
    const auto _tmp_rc = res::TransformIntoResult(res_expr); \
    if (R_FAILED(_tmp_rc)) { \
        return _tmp_rc; \
    } \
})
