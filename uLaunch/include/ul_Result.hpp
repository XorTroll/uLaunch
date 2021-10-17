
#include <switch.h>
#include <string>

// All 2380-**** results are from us

constexpr u32 Module = 380;
constexpr u32 SubmoduleOffset = 100;

#define RES_DEFINE_SUBMODULE(val) constexpr u32 Submodule = val

#define RES_DEFINE(name, val) constexpr Result Result ## name = MAKERESULT(Module + Submodule * SubmoduleOffset, val)

#ifndef R_TRY

#define R_TRY(res_expr) ({ \
    const auto _tmp_r_try_rc = static_cast<Result>(res_expr); \
    if (R_FAILED(_tmp_r_try_rc)) { \
        return _tmp_r_try_rc; \
    } \
})

#endif

constexpr Result ResultSuccess = 0;

namespace misc {

    RES_DEFINE_SUBMODULE(0);
    RES_DEFINE(InvalidJsonFile, 1);

}

namespace dmn {

    RES_DEFINE_SUBMODULE(1);
    RES_DEFINE(ApplicationActive, 1);
    RES_DEFINE(InvalidSelectedUser, 2);
    RES_DEFINE(AlreadyQueued, 3);
    RES_DEFINE(ApplicationNotActive, 4);

}

namespace menu {

    RES_DEFINE_SUBMODULE(2);
    RES_DEFINE(RomfsFileNotFound, 1);

}

namespace ipc {

    RES_DEFINE_SUBMODULE(3);
    RES_DEFINE(InvalidProcess, 1);

}

namespace res {

    Result GetResultByModuleAndName(const std::string &mod, const std::string &name);
    std::string GetModuleByResult(const Result rc);
    std::string GetNameByResult(const Result rc);

}