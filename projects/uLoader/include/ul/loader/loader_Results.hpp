
#pragma once
#include <ul/ul_Result.hpp>

namespace ul::loader {

    UL_RC_DEFINE_SUBMODULE(3);

    UL_RC_DEFINE(InvalidProcessType, 1);
    UL_RC_DEFINE(InvalidTargetInputMagic, 2);
    UL_RC_DEFINE(InvalidTargetInputSize, 3);

}