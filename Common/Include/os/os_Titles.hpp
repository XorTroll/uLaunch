
#pragma once
#include <q_Include.hpp>
#include <cfg/cfg_Config.hpp>

namespace os
{
    #define OS_MAX_TITLE_COUNT 64000
    #define OS_FLOG_APP_ID 0x01008BB00013C000

    inline constexpr bool IsFlogTitle(u64 app_id)
    {
        return (app_id == OS_FLOG_APP_ID);
    }

    std::vector<cfg::TitleRecord> QueryInstalledTitles(bool cache);
}