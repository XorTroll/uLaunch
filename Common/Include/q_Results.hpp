
#include <switch.h>
#include <vector>
#include <string>

// Result-making macros

#define RES_BLOCK_BEGIN static std::vector<ResultModuleImpl> BlockImpl = {

#define RES_MODULE_BEGIN(name, val) { val * 100, #name, {

#define RES_DEFINE(name, desc) { desc, #name },

#define RES_MODULE_END } },

#define RES_BLOCK_END };

#define RES_VALUE(module, name) res::GetResultByModuleAndName(#module, #name)
#define RES_DESCRIPTION(rc) res::GetDescriptionByResult(rc)

namespace res
{
    struct ResultImpl
    {
        u32 res_desc;
        std::string res_name;
    };

    struct ResultModuleImpl
    {
        u32 mod_val;
        std::string mod_name;
        std::vector<ResultImpl> results;
    };

    // All 2380-**** results are from uLaunch!
    static constexpr u32 Module = 380;

    RES_BLOCK_BEGIN

    RES_MODULE_BEGIN(Db, 1)
    RES_DEFINE(InvalidPasswordLength, 1)
    RES_DEFINE(PasswordNotFound, 2)
    RES_DEFINE(PasswordAlreadyExists, 3)
    RES_DEFINE(PasswordWriteFail, 4)
    RES_DEFINE(PasswordUserMismatch, 5)
    RES_DEFINE(PasswordMismatch, 6)
    RES_MODULE_END

    RES_MODULE_BEGIN(Misc, 2)
    RES_DEFINE(InvalidJSONFile, 1)
    RES_MODULE_END

    RES_MODULE_BEGIN(QDaemon, 3)
    RES_DEFINE(ApplicationActive, 1)
    RES_DEFINE(InvalidSelectedUser, 2)
    RES_DEFINE(AlreadyQueued, 3)
    RES_DEFINE(ApplicationNotActive, 4)
    RES_MODULE_END

    RES_BLOCK_END

    inline Result GetResultByModuleAndName(std::string mod, std::string name)
    {
        for(auto &module: BlockImpl)
        {
            if(module.mod_name == mod)
            {
                for(auto &res: module.results)
                { 
                    if(res.res_name == name)
                    {
                        return MAKERESULT(Module, module.mod_val + res.res_desc);
                    }
                }
            }
        }
        return 0;
    }

    inline std::string GetDescriptionByResult(Result rc)
    {
        for(auto &module: BlockImpl)
        {
            for(auto &res: module.results)
            { 
                auto resval = MAKERESULT(Module, module.mod_val + res.res_desc);
                if(resval == rc) return module.mod_name + " - " + res.res_name;
            }
        }
        return "";
    }
}