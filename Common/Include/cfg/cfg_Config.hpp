
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

        // Optional NACP params
        std::string name;
        std::string author;
        std::string version;
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

    struct Theme
    {
        std::string base_name;
        std::string path;
        ThemeManifest manifest;
    };

    struct RecordStrings
    {
        std::string name;
        std::string author;
        std::string version;
    };

    struct RecordInformation
    {
        RecordStrings strings;
        std::string icon_path;
    };

    struct Config
    {
        std::string theme_name;
        bool system_title_override_enabled;
        bool viewer_usb_enabled;

        JSON main_lang;
        JSON default_lang;
    };

    static constexpr u32 CurrentThemeFormatVersion = 1;

    #define CFG_THEME_DEFAULT "romfs:/default"
    #define CFG_LANG_DEFAULT "romfs:/LangDefault.json"
    #define CFG_CONFIG_JSON Q_BASE_SD_DIR "/config.json"

    TitleList LoadTitleList(bool cache);
    std::vector<TitleRecord> QueryAllHomebrew(std::string base = "sdmc:/switch");
    std::string GetRecordIconPath(TitleRecord record);
    RecordInformation GetRecordInformation(TitleRecord record);

    Theme LoadTheme(std::string base_name);
    std::vector<Theme> LoadThemes();
    std::string GetAssetByTheme(Theme &base, std::string resource_base);

    inline bool ThemeIsDefault(Theme &base)
    {
        return base.base_name.empty();
    }

    std::string GetLanguageJSONPath(std::string lang);
    std::string GetLanguageString(JSON &lang, JSON &def, std::string name);

    Config CreateNewAndLoadConfig();
    Config LoadConfig();
    Config EnsureConfig();
    void SaveConfig(Config &cfg);

    void SaveRecord(TitleRecord &record);
    void RemoveRecord(TitleRecord &record);
    bool MoveRecordTo(TitleList &list, TitleRecord record, std::string folder);
    TitleFolder &FindFolderByName(TitleList &list, std::string name);
    bool ExistsRecord(TitleList &list, TitleRecord record);

    std::string GetTitleCacheIconPath(u64 app_id);
    std::string GetNROCacheIconPath(std::string path);
}