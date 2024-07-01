
#pragma once
#include <ul/ul_Result.hpp>

namespace ul::sf {

    bool IsAvailable();

    Result Initialize();
    void Finalize();

    Result GetVersion(Version *out_ver);

}
