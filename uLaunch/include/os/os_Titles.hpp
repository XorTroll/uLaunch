
#pragma once
#include <ul_Include.hpp>
#include <cfg/cfg_Config.hpp>

namespace os {

    constexpr u32 MaxTitleCount = 64000;

    std::vector<cfg::TitleRecord> QueryInstalledTitles();

}