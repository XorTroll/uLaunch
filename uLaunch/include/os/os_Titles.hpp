
#pragma once
#include <ul_Include.hpp>
#include <cfg/cfg_Config.hpp>

namespace os
{
    static constexpr u32 MaxInstalledCount = 64000;

    std::vector<cfg::TitleRecord> QueryInstalledTitles();
}