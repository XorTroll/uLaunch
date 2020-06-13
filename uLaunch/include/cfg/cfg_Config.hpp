
#pragma once
#include <ul_Include.hpp>
#include <hb/hb_Target.hpp>
#include <fs/fs_Stdio.hpp>
#include <util/util_Convert.hpp>

namespace cfg {

    enum class TitleType : u32 {
        Invalid,
        Installed,
        Homebrew
    };

    struct TitleRecord {
        std::string json_name; // Empty for non-SD, normal title records
        u32 title_type;
        std::string sub_folder; // Empty for root, name for a certain folder
        std::string icon; // Custom icon, if specified

        u64 app_id; // TitleType::Installed
        hb::HbTargetParams nro_target; // TitleType::Homebrew

        // Optional NACP params
        std::string name;
        std::string author;
        std::string version;
    };

    struct TitleFolder {
        std::string name;
        std::vector<TitleRecord> titles;
    };

    struct TitleList {
        TitleFolder root;
        std::vector<TitleFolder> folders;
    };

    struct ThemeManifest {
        std::string name;
        u32 format_version;
        std::string release;
        std::string description;
        std::string author;
    };

    struct Theme {
        std::string base_name;
        std::string path;
        ThemeManifest manifest;
    };

    struct RecordStrings {
        std::string name;
        std::string author;
        std::string version;
    };

    struct RecordInformation {
        RecordStrings strings;
        std::string icon_path;
    };

    // Take over eShop by default
    static constexpr u64 DefaultMenuProgramId = 0x010000000000100B;

    // Take over parental controls applet by default
    static constexpr u64 DefaultHomebrewAppletProgramId = 0x0100000000001001;

    struct Config {
        std::string theme_name;
        bool system_title_override_enabled;
        bool viewer_usb_enabled;
        u64 menu_program_id;
        u64 homebrew_applet_program_id;
        u64 homebrew_title_application_id;

        JSON main_lang;
        JSON default_lang;

        Config() : system_title_override_enabled(false), viewer_usb_enabled(false), menu_program_id(DefaultMenuProgramId), homebrew_applet_program_id(DefaultHomebrewAppletProgramId), homebrew_title_application_id(0) {}

    };

    static constexpr u32 CurrentThemeFormatVersion = 1;

    #define CFG_THEME_DEFAULT "romfs:/default"
    #define CFG_LANG_DEFAULT "romfs:/LangDefault.json"
    #define CFG_CONFIG_JSON UL_BASE_SD_DIR "/config.json"

    TitleList LoadTitleList();
    std::vector<TitleRecord> QueryAllHomebrew(const std::string &base = "sdmc:/switch");
    void CacheEverything(const std::string &hb_base_path = "sdmc:/switch");
    std::string GetRecordIconPath(TitleRecord record);
    RecordInformation GetRecordInformation(TitleRecord record);

    Theme LoadTheme(const std::string &base_name);
    std::vector<Theme> LoadThemes();
    std::string GetAssetByTheme(const Theme &base, const std::string &resource_base);

    inline bool ThemeIsDefault(const Theme &base) {
        return base.base_name.empty();
    }

    inline std::string GetLanguageJSONPath(const std::string &lang) {
        return UL_BASE_SD_DIR "/lang/" + lang + ".json";
    }

    std::string GetLanguageString(const JSON &lang, const JSON &def, const std::string &name);

    Config CreateNewAndLoadConfig();
    Config LoadConfig();
    
    inline Config EnsureConfig() {
        if(fs::ExistsFile(CFG_CONFIG_JSON)) {
            return LoadConfig();
        }
        else {
            return CreateNewAndLoadConfig();
        }
    }

    void SaveConfig(const Config &cfg);

    void SaveRecord(TitleRecord &record);
    void RemoveRecord(TitleRecord &record);
    bool MoveRecordTo(TitleList &list, TitleRecord record, const std::string &folder);
    TitleFolder &FindFolderByName(TitleList &list, const std::string &name);
    void RenameFolder(TitleList &list, const std::string &old_name, const std::string &new_name);
    bool ExistsRecord(TitleList &list, TitleRecord record);

    inline std::string GetTitleCacheIconPath(u64 app_id) {
        return UL_BASE_SD_DIR "/title/" + util::FormatApplicationId(app_id) + ".jpg";
    }

    std::string GetNROCacheIconPath(const std::string &path);

}