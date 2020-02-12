
#include <switch.h>
#include <string>

// Result-making macros

#define RES_VALUE(module, name) res::GetResultByModuleAndName(#module, #name)
#define RES_DESCRIPTION(rc) res::GetDescriptionByResult(rc)

namespace res
{
    // All 2380-**** results are from uLaunch!
    static constexpr u32 Module = 380;

    Result GetResultByModuleAndName(std::string mod, std::string name);
    std::string GetDescriptionByResult(Result rc);
}