
#pragma once
#include <dmi/dmi_DaemonMenuInteraction.hpp>

namespace am {

    Result ReadDataFromStorage(void *data, size_t data_size);
    Result ReadStartMode(dmi::MenuStartMode &out_start_mode);

}