
#pragma once
#include <ul/ul_Include.hpp>
#include <functional>

namespace ul::man {

    constexpr const char TemporaryReleaseZipPath[] = "sdmc:/ulaunch_tmp.zip";

    using RetrieveOnProgressCallback = std::function<void(const double now_downloaded, const double total_to_download)>;

    std::string RetrieveContent(const std::string &url, const std::string &mime_type = "");
    void RetrieveToFile(const std::string &url, const std::string &path, RetrieveOnProgressCallback on_progress_cb);
    bool HasConnection();

}
