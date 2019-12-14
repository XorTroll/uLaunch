
#pragma once
#include <q_Include.hpp>

namespace hb
{
    struct TargetInput
    {
        char nro_path[2048];
        char argv[2048];
    };

    void Target(TargetInput input, bool once); // Only used by QHbTarget processes
}