
#pragma once
#include <vector>
#include <ul/cfg/cfg_Config.hpp>

namespace ul::os {

    constexpr u32 MaxTitleCount = 64000;

    std::vector<cfg::TitleRecord> QueryInstalledTitles();

}