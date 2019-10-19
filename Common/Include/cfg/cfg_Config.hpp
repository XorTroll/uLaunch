
#pragma once
#include <q_Include.hpp>
#include <hb/hb_Target.hpp>

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
        std::string icon; // Custom icon, if specified

        u64 app_id; // TitleType::Installed
        hb::TargetInput nro_target; // TitleType::Homebrew
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
    };

    struct ProcessedTheme
    {
        Theme base;
        UIConfig ui;
        SoundConfig sound;
    };

    struct RecordInformation
    {
        NacpStruct nacp;
        std::string icon_path;
    };

    struct MenuConfig
    {
        std::string theme_name;
    };

    static constexpr u32 CurrentThemeFormatVersion = 0;

    #define CFG_THEME_DEFAULT "romfs:/default"
    #define CFG_CONFIG_JSON Q_BASE_SD_DIR "/config.json"

    void CacheHomebrew(std::string nro_path);
    ResultWith<TitleList> LoadTitleList(bool cache);
    std::vector<TitleRecord> QueryAllHomebrew(bool cache, std::string base = "sdmc:/switch");
    std::string GetRecordIconPath(TitleRecord record);
    RecordInformation GetRecordInformation(TitleRecord record);
    NacpLanguageEntry *GetRecordInformationLanguageEntry(RecordInformation &info);

    Theme LoadTheme(std::string base_name);
    std::vector<Theme> LoadThemes();
    std::string ThemeResource(Theme &base, std::string resource_base);
    std::string ProcessedThemeResource(ProcessedTheme &base, std::string resource_base);
    ProcessedTheme ProcessTheme(Theme &base);

    MenuConfig CreateNewAndLoadConfig();
    MenuConfig LoadConfig();
    MenuConfig EnsureConfig();
    void SaveConfig(MenuConfig &cfg);

    void SaveRecord(TitleRecord &record);
    bool MoveRecordTo(TitleList &list, TitleRecord record, std::string folder);
    TitleFolder &FindFolderByName(TitleList &list, std::string name);
    bool ExistsRecord(TitleList &list, TitleRecord record);

    std::string GetTitleCacheIconPath(u64 app_id);
    std::string GetNROCacheIconPath(std::string path);
}