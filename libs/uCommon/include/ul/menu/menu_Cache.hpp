
#pragma once
#include <ul/ul_Include.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_String.hpp>

namespace ul::menu {

    void CacheHomebrew(const std::string &hb_base_path = RootHomebrewPath);
    void CacheHomebrewEntry(const std::string &nro_path);

    void CacheApplications(const std::vector<NsApplicationRecord> &records);
    bool CacheSingleApplication(const u64 app_id);

    std::string GetHomebrewCacheIconPath(const std::string &nro_path);
    std::string GetHomebrewCacheNacpPath(const std::string &nro_path);

}
