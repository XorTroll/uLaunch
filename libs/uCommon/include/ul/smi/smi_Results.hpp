
#pragma once
#include <ul/ul_Result.hpp>

namespace ul::smi {

    UL_RC_DEFINE_SUBMODULE(1);
    
    UL_RC_DEFINE(OutOfPushSpace, 1);
    UL_RC_DEFINE(OutOfPopSpace, 2);
    UL_RC_DEFINE(InvalidInHeaderMagic, 3);
    UL_RC_DEFINE(InvalidOutHeaderMagic, 4);
    UL_RC_DEFINE(WaitTimeout, 5);

}