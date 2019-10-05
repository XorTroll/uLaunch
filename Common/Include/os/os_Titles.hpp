
#pragma once
#include <q_Include.hpp>
#include <cfg/cfg_Config.hpp>

namespace os
{
    #define OS_MAX_TITLE_COUNT 64000

    ResultWith<std::vector<cfg::TitleRecord>> QueryInstalledTitles(bool cache);
}