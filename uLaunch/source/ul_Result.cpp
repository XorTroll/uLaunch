
#include <ul_Result.hpp>

namespace res {

    namespace {

        struct ResultInfoImpl {
            Result rc;
            const char *mod;
            const char *name;
        };

        #define _UL_RC_INFO_DEFINE(mod, name) { mod::Result ## name, #mod, #name }
        constexpr ResultInfoImpl g_ResultInfoTableImpl[] = {
            _UL_RC_INFO_DEFINE(misc, AssertionFailed),
            _UL_RC_INFO_DEFINE(misc, UnexpectedTransform),
            _UL_RC_INFO_DEFINE(misc, InvalidJsonFile),

            _UL_RC_INFO_DEFINE(dmn, ApplicationActive),
            _UL_RC_INFO_DEFINE(dmn, InvalidSelectedUser),
            _UL_RC_INFO_DEFINE(dmn, AlreadyQueued),
            _UL_RC_INFO_DEFINE(dmn, ApplicationNotActive),

            _UL_RC_INFO_DEFINE(menu, RomfsFileNotFound),

            _UL_RC_INFO_DEFINE(ipc, InvalidProcess),

            _UL_RC_INFO_DEFINE(dmi, OutOfPushSpace),
            _UL_RC_INFO_DEFINE(dmi, OutOfPopSpace),
            _UL_RC_INFO_DEFINE(dmi, InvalidInHeaderMagic),
            _UL_RC_INFO_DEFINE(dmi, InvalidOutHeaderMagic),
        };
        #undef _UL_RC_INFO_DEFINE
        constexpr size_t ResultInfoTableImplCount = sizeof(g_ResultInfoTableImpl) / sizeof(ResultInfoImpl);

        inline constexpr ResultInfoImpl GetResultInfo(const u32 idx) {
            return g_ResultInfoTableImpl[idx];
        }

    }

    Result GetResultByModuleAndName(const std::string &mod, const std::string &name) {
        for(u32 i = 0; i < ResultInfoTableImplCount; i++) {
            const auto rc_info = GetResultInfo(i);
            if((mod == rc_info.mod) && (name == rc_info.name)) {
                return rc_info.rc;
            }
        }
        
        // TODO: proper return value?
        return 0;
    }

    std::string GetModuleByResult(const Result rc) {
        for(u32 i = 0; i < ResultInfoTableImplCount; i++) {
            const auto rc_info = GetResultInfo(i);
            if(rc == rc_info.rc) {
                return rc_info.mod;
            }
        }

        return "";
    }

    std::string GetNameByResult(const Result rc) {
        for(u32 i = 0; i < ResultInfoTableImplCount; i++) {
            const auto rc_info = GetResultInfo(i);
            if(rc == rc_info.rc) {
                return rc_info.name;
            }
        }

        return "";
    }

}