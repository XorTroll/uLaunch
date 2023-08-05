#include <ul/cfg/cfg_Config.hpp>
#include <ul/util/util_String.hpp>
#include <ul/util/util_Stl.hpp>
#include <ul/os/os_Applications.hpp>
#include <ul/ul_Result.hpp>

namespace ul::cfg {

    Theme LoadTheme(const std::string &base_name) {
        Theme theme = {
            .base_name = base_name
        };
        auto theme_dir = fs::JoinPath(ThemesPath, base_name);
        auto manifest_path = fs::JoinPath(theme_dir, "theme/Manifest.json");
        if(base_name.empty() || !fs::ExistsFile(manifest_path)) {
            theme_dir = DefaultThemePath;
        }
        manifest_path = fs::JoinPath(theme_dir, "theme/Manifest.json");

        util::JSON manifest_json;
        // TODONEW: error checking, make some fields required
        if(R_SUCCEEDED(util::LoadJSONFromFile(manifest_json, manifest_path))) {
            theme.manifest.name = manifest_json.value("name", "'" + base_name + "'");
            theme.manifest.format_version = manifest_json.value("format_version", 0);
            theme.manifest.release = manifest_json.value("release", "");
            theme.manifest.description = manifest_json.value("description", "");
            theme.manifest.author = manifest_json.value("author", "");
            theme.path = theme_dir;
            return theme;
        }

        return LoadTheme("");
    }

    std::vector<Theme> LoadThemes() {
        std::vector<Theme> themes;
        UL_FS_FOR(ThemesPath, name, path, is_dir, is_file, {
            const auto theme = LoadTheme(name);
            if(!theme.path.empty()) {
                themes.push_back(theme);
            }
        });
        return themes;
    }

    std::string GetAssetByTheme(const Theme &base, const std::string &resource_base) {
        auto base_res = fs::JoinPath(base.path, resource_base);
        if(fs::ExistsFile(base_res)) {
            return base_res;
        }

        base_res = fs::JoinPath(DefaultThemePath, resource_base);
        if(fs::ExistsFile(base_res)) {
            return base_res;
        }

        return "";
    }

    std::string GetLanguageString(const util::JSON &lang, const util::JSON &def, const std::string &name) {
        auto str = lang.value(name, "");
        if(str.empty()) {
            str = def.value(name, "");
        }
        return str;
    }

    Config CreateNewAndLoadConfig() {
        const Config empty_cfg = {};
        SaveConfig(empty_cfg);
        return empty_cfg;
    }

    Config LoadConfig() {
        Config cfg = {};
        const auto cfg_file_size = fs::GetFileSize(ConfigPath);
        auto cfg_file_buf = new u8[cfg_file_size]();
        if(fs::ReadFile(ConfigPath, cfg_file_buf, cfg_file_size)) {
            size_t cur_offset = 0;
            const auto cfg_header = *reinterpret_cast<ConfigHeader*>(cfg_file_buf);
            if(cfg_header.magic == ConfigHeader::Magic) {
                cur_offset += sizeof(ConfigHeader);
                if(cur_offset <= cfg_file_size) {
                    cfg.entries.reserve(cfg_header.entry_count);
                    for(u32 i = 0; i < cfg_header.entry_count; i++) {
                        ConfigEntry ent = {};
                        ent.header = *reinterpret_cast<ConfigEntryHeader*>(cfg_file_buf + cur_offset);
                        cur_offset += sizeof(ConfigEntryHeader);
                        if(cur_offset > cfg_file_size) {
                            break;
                        }
                        switch(ent.header.type) {
                            case ConfigEntryType::Bool: {
                                if(ent.header.size != sizeof(bool)) {
                                    return CreateNewAndLoadConfig();
                                }
                                ent.bool_value = *reinterpret_cast<bool*>(cfg_file_buf + cur_offset);
                                break;
                            }
                            case ConfigEntryType::U64: {
                                if(ent.header.size != sizeof(u64)) {
                                    return CreateNewAndLoadConfig();
                                }
                                ent.u64_value = *reinterpret_cast<u64*>(cfg_file_buf + cur_offset);
                                break;
                            }
                            case ConfigEntryType::String: {
                                if(ent.header.size == 0) {
                                    ent.str_value = "";
                                }
                                else {
                                    ent.str_value = std::string(reinterpret_cast<char*>(cfg_file_buf + cur_offset), ent.header.size);
                                }
                                break;
                            }
                        }
                        cur_offset += ent.header.size;
                        if(cur_offset > cfg_file_size) {
                            break;
                        }
                        cfg.entries.push_back(std::move(ent));
                    }
                    if(cur_offset <= cfg_file_size) {
                        return cfg;
                    }
                }
            }
        }
        return CreateNewAndLoadConfig();
    }

    void SaveConfig(const Config &cfg) {
        const ConfigHeader cfg_header = {
            .magic = ConfigHeader::Magic,
            .entry_count = static_cast<u32>(cfg.entries.size())
        };
        fs::WriteFile(ConfigPath, &cfg_header, sizeof(cfg_header), true);
        for(const auto &entry : cfg.entries) {
            fs::WriteFile(ConfigPath, &entry.header, sizeof(entry.header), false);
            switch(entry.header.type) {
                case ConfigEntryType::Bool: {
                    fs::WriteFile(ConfigPath, &entry.bool_value, sizeof(entry.bool_value), false);
                    break;
                }
                case ConfigEntryType::U64: {
                    fs::WriteFile(ConfigPath, &entry.u64_value, sizeof(entry.u64_value), false);
                    break;
                }
                case ConfigEntryType::String: {
                    fs::WriteFile(ConfigPath, entry.str_value.c_str(), entry.str_value.length(), false);
                    break;
                }
            }
        }
    }

}