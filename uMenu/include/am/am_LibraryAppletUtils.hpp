
#pragma once
#include <dmi/dmi_DaemonMenuInteraction.hpp>

namespace am {

    Result ReadDataFromStorage(void *out_data, const size_t data_size);
    Result ReadStartMode(dmi::MenuStartMode &out_start_mode);

}