
#pragma once
#include <q_Include.hpp>

namespace cfg
{
    enum class TitleType : u32
    {
        Installed = BIT(0),
        Homebrew = BIT(1)
    };

    struct TitleRecord
    {
        u32 title_type;
        std::string sub_folder; // Empty for root, name for a certain folder

        u64 app_id; // TitleType::Installed
        std::string nro_path; // TitleType::Homebrew
    };

    struct TitleFolder
    {
        std::string name;
        std::vector<TitleRecord> titles;
    };

    struct TitleList
    {
        TitleFolder root;
        std::vector<TitleFolder> folders;
    };

    ResultWith<TitleList> LoadTitleList(bool cache);

    bool MoveTitleToDirectory(TitleList &list, u64 app_id, std::string dir);

    std::string GetTitleCacheIconPath(u64 app_id);
    std::string GetNROCacheIconPath(std::string path);
}