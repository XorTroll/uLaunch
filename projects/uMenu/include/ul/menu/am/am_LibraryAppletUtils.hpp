
#pragma once
#include <ul/smi/smi_Protocol.hpp>

namespace ul::menu::am {

    Result ReadFromInputStorage(void *out_data, const size_t data_size);
    Result ReadStartMode(smi::MenuStartMode &out_start_mode);

}
