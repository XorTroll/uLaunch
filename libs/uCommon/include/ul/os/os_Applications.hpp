
#pragma once
#include <vector>
#include <ul/cfg/cfg_Config.hpp>

namespace ul::os {

    constexpr u32 MaxApplicationCount = 64000;

    std::vector<NsApplicationRecord> ListApplicationRecords();
    NsApplicationContentMetaStatus GetApplicationContentMetaStatus(const u64 app_id);

}