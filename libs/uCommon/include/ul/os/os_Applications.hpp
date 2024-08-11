
#pragma once
#include <vector>
#include <ul/cfg/cfg_Config.hpp>

namespace ul::os {

    enum class ApplicationViewFlag : u32 {
        Valid = BIT(0),
        GameCardApplication = BIT(6),
        GameCardApplicationAccessible = BIT(7),
        Launchable = BIT(8),
        NeedsVerify = BIT(13)
    };

    std::vector<NsApplicationRecord> ListApplicationRecords();
    std::vector<NsApplicationView> ListApplicationViews(const std::vector<NsApplicationRecord> &base_records);

}
