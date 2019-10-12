
#pragma once
#include <q_Include.hpp>

namespace cfg
{
    enum class TitleType : u32
    {
        Invalid,
        Installed,
        Homebrew
    };

    struct TitleRecord
    {
        std::string json_name; // Empty for non-SD, normal title records
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

    struct ThemeManifest
    {
        std::string name;
        u32 format_version;
        std::string release;
        std::string description;
        std::string author;
    };

    struct UIConfig
    {
        u8 suspended_final_alpha;
    };

    struct SoundConfig
    {
        bool loop;
        bool fade_in;
        bool fade_out;
    };

    struct Theme
    {
        std::string path;
        ThemeManifest manifest;
        UIConfig ui;
        SoundConfig sound;
    };

    struct RecordInformation
    {
        NacpStruct nacp;
        std::string icon_path;
    };

    void CacheHomebrew(std::string nro_path);
    ResultWith<TitleList> LoadTitleList(bool cache);
    std::vector<TitleRecord> QueryAllHomebrew(bool cache, std::string base = "sdmc:/switch");
    RecordInformation GetRecordInformation(TitleRecord record);

    void SaveRecord(TitleRecord record);
    bool MoveTitleToDirectory(TitleList &list, u64 app_id, std::string dir);
    TitleFolder &FindFolderByName(TitleList &list, std::string name);

    std::string GetTitleCacheIconPath(u64 app_id);
    std::string GetNROCacheIconPath(std::string path);
}