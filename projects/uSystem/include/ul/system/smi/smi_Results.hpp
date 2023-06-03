
#pragma once
#include <ul/ul_Result.hpp>

namespace ul::system::smi {

    UL_RC_DEFINE_SUBMODULE(4);

    UL_RC_DEFINE(ApplicationActive, 1);
    UL_RC_DEFINE(InvalidSelectedUser, 2);
    UL_RC_DEFINE(AlreadyQueued, 3);
    UL_RC_DEFINE(ApplicationNotActive, 4);
    UL_RC_DEFINE(NoHomebrewTakeoverApplication, 5);

}