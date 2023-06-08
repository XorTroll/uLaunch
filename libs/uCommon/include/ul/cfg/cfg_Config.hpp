
#pragma once
#include <ul/ul_Include.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/loader/loader_TargetInput.hpp>
#include <ul/util/util_String.hpp>
#include <ul/util/util_Json.hpp>

namespace ul::cfg {

    enum class TitleType : u32 {
        Invalid,
        Installed,
        Homebrew
    };

    struct TitleRecord {
        std::string json_name; // Empty for non-SD, normal title records
        TitleType title_type; // Title type
        std::string sub_folder; // Empty for root, name for a certain folder
        std::string icon; // Custom icon, if specified

        u64 app_id; // For TitleType::Installed
        loader::TargetInput nro_target; // For TitleType::Homebrew

        // Optional NACP params
        std::string name;
        std::string author;
        std::string version;

        inline bool Equals(const TitleRecord &other) const {
            if(this->title_type == other.title_type) {
                switch(this->title_type) {
                    case TitleType::Installed: {
                        return this->app_id == other.app_id;
                    }
                    case TitleType::Homebrew: {
                        return std::strcmp(this->nro_target.nro_path, other.nro_target.nro_path) == 0;
                    }
                    default: {
                        return false;
                    }
                }
            }
            return false;
        }
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

        inline bool IsDefault() {
            return this->base_name.empty();
        }
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

    enum class ConfigEntryId : u8 {
        MenuTakeoverProgramId,
        HomebrewAppletTakeoverProgramId,
        HomebrewApplicationTakeoverApplicationId,
        ViewerUsbEnabled,
        ActiveThemeName
    };

    enum class ConfigEntryType : u8 {
        Bool,
        U64,
        String
    };

    struct ConfigEntryHeader {
        ConfigEntryId id;
        ConfigEntryType type;
        u8 size;
        u8 pad;
    };

    struct ConfigEntry {
        ConfigEntryHeader header;
        bool bool_value;
        u64 u64_value;
        std::string str_value;

        template<typename T>
        inline bool Get(T &out_t) const {
            switch(this->header.type) {
                case ConfigEntryType::Bool: {
                    if constexpr(std::is_same_v<T, bool>) {
                        out_t = this->bool_value;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryType::U64: {
                    if constexpr(std::is_same_v<T, u64>) {
                        out_t = this->u64_value;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryType::String: {
                    if constexpr(std::is_same_v<T, std::string>) {
                        out_t = this->str_value;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
            }
            return false;
        }

        template<typename T>
        inline bool Set(const T &t) {
            switch(this->header.type) {
                case ConfigEntryType::Bool: {
                    if constexpr(std::is_same_v<T, bool>) {
                        this->bool_value = t;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryType::U64: {
                    if constexpr(std::is_same_v<T, u64>) {
                        this->u64_value = t;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryType::String: {
                    if constexpr(std::is_same_v<T, std::string>) {
                        this->str_value = t;
                        this->header.size = this->str_value.length();
                        return true;
                    }
                    else {
                        return false;
                    }
                }
            }
            return false;
        }
    };

    struct ConfigHeader {
        u32 magic;
        u32 entry_count;

        static constexpr u32 Magic = 0x47464355; // "UCFG"
    };

    struct Config {
        std::vector<ConfigEntry> entries;

        template<typename T>
        inline bool SetEntry(const ConfigEntryId id, const T &t) {
            for(auto &entry : this->entries) {
                if(entry.header.id == id) {
                    return entry.Set(t);
                }
            }
            // Create new entry
            ConfigEntry new_entry = {
                .header = {
                    .id = id
                }
            };
            switch(id) {
                case ConfigEntryId::MenuTakeoverProgramId:
                case ConfigEntryId::HomebrewAppletTakeoverProgramId:
                case ConfigEntryId::HomebrewApplicationTakeoverApplicationId: {
                    if constexpr(std::is_same_v<T, u64>) {
                        new_entry.header.type = ConfigEntryType::U64;
                        new_entry.header.size = sizeof(t);
                        new_entry.u64_value = t;
                        break;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::ViewerUsbEnabled: {
                    if constexpr(std::is_same_v<T, bool>) {
                        new_entry.header.type = ConfigEntryType::Bool;
                        new_entry.header.size = sizeof(t);
                        new_entry.bool_value = t;
                        break;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::ActiveThemeName: {
                    if constexpr(std::is_same_v<T, std::string>) {
                        new_entry.header.type = ConfigEntryType::String;
                        new_entry.header.size = t.length();
                        new_entry.str_value = t;
                        break;
                    }
                    else {
                        return false;
                    }
                }
            }
            this->entries.push_back(std::move(new_entry));
            return true;
        }
        
        template<typename T>
        inline bool GetEntry(const ConfigEntryId id, T &out_t) const {
            for(const auto &entry : this->entries) {
                if(entry.header.id == id) {
                    return entry.Get(out_t);
                }
            }

            // Default values
            switch(id) {
                case ConfigEntryId::MenuTakeoverProgramId: {
                    if constexpr(std::is_same_v<T, u64>) {
                        // Take over eShop by default
                        out_t = 0x010000000000100B;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::HomebrewAppletTakeoverProgramId: {
                    if constexpr(std::is_same_v<T, u64>) {
                        // Take over parental control applet by default
                        out_t = 0x0100000000001001;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::HomebrewApplicationTakeoverApplicationId: {
                    if constexpr(std::is_same_v<T, u64>) {
                        // No donor title by default
                        out_t = 0;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::ViewerUsbEnabled: {
                    if constexpr(std::is_same_v<T, bool>) {
                        // Disabled by default, it might interfer with other homebrews
                        out_t = false;
                        return true;
                    }
                    else {
                        return false;
                    }
                }
                case ConfigEntryId::ActiveThemeName: {
                    if constexpr(std::is_same_v<T, std::string>) {
                        // Empty by default
                        out_t = "";
                        return true;
                    }
                    else {
                        return false;
                    }
                }
            }
            return false;
        }
    };

    constexpr u32 CurrentThemeFormatVersion = 1;

    TitleList LoadTitleList();
    std::vector<TitleRecord> QueryAllHomebrew(const std::string &base = RootHomebrewPath);
    void CacheEverything(const std::string &hb_base_path = RootHomebrewPath);

    std::string GetRecordIconPath(const TitleRecord &record);
    std::string GetRecordJsonPath(const TitleRecord &record);
    RecordInformation GetRecordInformation(const TitleRecord &record);

    Theme LoadTheme(const std::string &base_name);
    std::vector<Theme> LoadThemes();
    std::string GetAssetByTheme(const Theme &base, const std::string &resource_base);

    inline std::string GetLanguageJSONPath(const std::string &lang) {
        return "sdmc:/ulaunch/lang/" + lang + ".json";
    }

    std::string GetLanguageString(const util::JSON &lang, const util::JSON &def, const std::string &name);

    Config CreateNewAndLoadConfig();
    Config LoadConfig();

    void SaveConfig(const Config &cfg);

    void SaveRecord(const TitleRecord &record);
    
    inline void RemoveRecord(const TitleRecord &record) {
        fs::DeleteFile(cfg::GetRecordJsonPath(record));
    }

    bool MoveRecordTo(TitleList &list, const TitleRecord &record, const std::string &folder);
    TitleFolder &FindFolderByName(TitleList &list, const std::string &name);
    void RenameFolder(TitleList &list, const std::string &old_name, const std::string &new_name);
    bool ExistsRecord(const TitleList &list, const TitleRecord &record);

    inline std::string GetTitleCacheIconPath(const u64 app_id) {
        return "sdmc:/ulaunch/titles/" + util::FormatProgramId(app_id) + ".jpg";
    }

    std::string GetNroCacheIconPath(const std::string &path);

}