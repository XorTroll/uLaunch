
#include <ul_Results.hpp>
#include <vector>

// Result-making macros

#define RES_BLOCK_BEGIN static std::vector<ResultModuleImpl> g_result_impl_table = {

#define RES_MODULE_BEGIN(name, val) { val * 100, #name, {

#define RES_DEFINE(name, desc) { desc, #name },

#define RES_MODULE_END } },

#define RES_BLOCK_END };

namespace res {

    struct ResultImpl {
        u32 res_desc;
        std::string res_name;
    };

    struct ResultModuleImpl {
        u32 mod_val;
        std::string mod_name;
        std::vector<ResultImpl> results;
    };

    RES_BLOCK_BEGIN

    RES_MODULE_BEGIN(Misc, 1)
    RES_DEFINE(InvalidJSONFile, 1)
    RES_MODULE_END

    RES_MODULE_BEGIN(Daemon, 2)
    RES_DEFINE(ApplicationActive, 1)
    RES_DEFINE(InvalidSelectedUser, 2)
    RES_DEFINE(AlreadyQueued, 3)
    RES_DEFINE(ApplicationNotActive, 4)
    RES_DEFINE(PrivateServiceInvalidProcess, 5)
    RES_MODULE_END

    RES_MODULE_BEGIN(Menu, 3)
    RES_DEFINE(RomfsBinNotFound, 1)
    RES_MODULE_END

    RES_BLOCK_END

    Result GetResultByModuleAndName(std::string mod, std::string name) {
        for(auto &module: g_result_impl_table) {
            if(module.mod_name == mod) {
                for(auto &res: module.results) { 
                    if(res.res_name == name) {
                        return MAKERESULT(Module, module.mod_val + res.res_desc);
                    }
                }
            }
        }
        return 0;
    }

    std::string GetDescriptionByResult(Result rc) {
        for(auto &module: g_result_impl_table) {
            for(auto &res: module.results) {
                if(rc == MAKERESULT(Module, module.mod_val + res.res_desc)) {
                    return module.mod_name + " - " + res.res_name;
                }
            }
        }
        return "Unknown result";
    }

}