#include <ul/cfg/cfg_Config.hpp>
#include <ul/util/util_String.hpp>
#include <ul/os/os_Applications.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/util/util_Zip.hpp>

namespace ul::cfg {

    namespace {

        constexpr auto ThemeManifestPath = "theme/Manifest.json";

    }

    Result TryLoadTheme(const std::string &theme_name, Theme &out_theme) {
        const auto theme_path = fs::JoinPath(ThemesPath, theme_name);
        auto theme_zip = zip_open(theme_path.c_str(), 0, 'r');
        if(!theme_zip) {
            return ResultInvalidThemeZipFile;
        }
        UL_ON_SCOPE_EXIT(
            zip_close(theme_zip);
        );

        if(zip_entry_open(theme_zip, ThemeManifestPath) != 0) {
            return ResultThemeManifestNotFound;
        }

        void *manifest_json_ptr;
        size_t manifest_json_ptr_size;
        if(zip_entry_read(theme_zip, &manifest_json_ptr, &manifest_json_ptr_size) <= 0) {
            return ResultInvalidThemeZipFileRead;
        }
        std::string manifest_json_str(reinterpret_cast<char*>(manifest_json_ptr), manifest_json_ptr_size);
        free(manifest_json_ptr);
        zip_entry_close(theme_zip);

        const auto manifest_json = util::JSON::parse(manifest_json_str);

        #define _THEME_MANIFEST_CHECK_GET_VALUE(name, type, rc) { \
            if(!manifest_json.count(#name)) { \
                return rc; \
            } \
            out_theme.manifest.name = manifest_json[#name].get<type>(); \
        }

        _THEME_MANIFEST_CHECK_GET_VALUE(format_version, u32, ResultThemeManifestVersionNotFound)
        _THEME_MANIFEST_CHECK_GET_VALUE(name, std::string, ResultThemeManifestNameNotFound)
        _THEME_MANIFEST_CHECK_GET_VALUE(author, std::string, ResultThemeManifestAuthorNotFound)
        _THEME_MANIFEST_CHECK_GET_VALUE(description, std::string, ResultThemeManifestDescriptionNotFound)
        _THEME_MANIFEST_CHECK_GET_VALUE(release, std::string, ResultThemeManifestReleaseNotFound)

        out_theme.name = theme_name;
        return ResultSuccess;
    }

    Result TryCacheLoadThemeIcon(const Theme &theme, std::string &out_icon_path) {
        const auto theme_path = fs::JoinPath(ThemesPath, theme.name);
        auto theme_zip = zip_open(theme_path.c_str(), 0, 'r');
        if(!theme_zip) {
            return ResultInvalidThemeZipFile;
        }
        UL_ON_SCOPE_EXIT(
            zip_close(theme_zip);
        );

        std::string icon_fmt;
        for(const auto &fmt: ImageFormatList) {
            const auto icon_path = fs::JoinPath("theme", "Icon." + std::string(fmt));
            if(zip_entry_open(theme_zip, icon_path.c_str()) == 0) {
                icon_fmt = fmt;
                break;
            }
        }
        if(icon_fmt.empty()) {
            return ResultThemeIconNotFound;
        }

        void *icon_ptr;
        size_t icon_ptr_size;
        if(zip_entry_read(theme_zip, &icon_ptr, &icon_ptr_size) <= 0) {
            return ResultInvalidThemeZipFileRead;
        }

        u8 hash[SHA256_HASH_SIZE] = {};
        sha256CalculateHash(hash, icon_ptr, icon_ptr_size);
        const auto icon_cache_path = fs::JoinPath(ThemePreviewCachePath, util::FormatSha256Hash(hash, true) + "." + icon_fmt);

        fs::CreateDirectory(ThemePreviewCachePath);
        if(!fs::WriteFile(icon_cache_path, icon_ptr, icon_ptr_size, true)) {
            return ResultThemeIconCacheFail;
        }

        free(icon_ptr);
        zip_entry_close(theme_zip);
        out_icon_path = icon_cache_path;
        return ResultSuccess;
    }

    std::vector<Theme> FindThemes() {
        std::vector<Theme> themes;
        Theme cur_theme;
        UL_FS_FOR(ThemesPath, name, path, is_dir, is_file, {
            if(is_file) {
                const auto rc = TryLoadTheme(name, cur_theme);
                if(R_SUCCEEDED(rc)) {
                    if(!IsThemeOutdated(cur_theme)) {
                        themes.push_back(cur_theme);
                    }
                    else {
                        UL_LOG_WARN("Outdated theme file '%s': theme version %d != current version %d", name.c_str(), cur_theme.manifest.format_version, CurrentThemeFormatVersion);
                    }
                }
                else {
                    UL_LOG_WARN("Invalid theme file '%s': %s", name.c_str(), util::FormatResultDisplay(rc).c_str());
                }
            }
        });
        return themes;
    }

    void CacheActiveTheme(const Config &cfg) {
        RemoveActiveThemeCache();

        std::string active_theme_name;
        if(!cfg.GetEntry(ConfigEntryId::ActiveThemeName, active_theme_name)) {
            UL_LOG_WARN("Unable to get active theme name from config...");
        }
        if(active_theme_name.empty()) {
            // Assume there is no custom theme
            return;
        }

        const auto active_theme_path = fs::JoinPath(ThemesPath, active_theme_name);
        auto zip_file = zip_open(active_theme_path.c_str(), 0, 'r');
        if(zip_file == nullptr) {
            UL_LOG_WARN("Unable to open theme path for cache... is the theme deleted?");
            return;
        }

        const auto file_count = zip_entries_total(zip_file);
        if(file_count > 0) {
            for(u32 i = 0; i < file_count; i++) {
                const auto zip_rc = zip_entry_openbyindex(zip_file, i);
                if(zip_rc != 0) {
                    UL_LOG_WARN("Unable to open theme zip file index %d (from %d total): err %d", i, file_count, zip_rc);
                    continue;
                }

                const auto entry_path = fs::JoinPath(ActiveThemeCachePath, zip_entry_name(zip_file));
                const auto is_dir = zip_entry_isdir(zip_file);
                if(is_dir) {
                    fs::CreateDirectory(entry_path);
                }
                else {
                    void *read_buf;
                    size_t read_buf_size;
                    const auto read_size = zip_entry_read(zip_file, &read_buf, &read_buf_size);
                    if(read_size <= 0) {
                        UL_LOG_WARN("Unable to read theme zip file index %d (from %d total): err %d", i, file_count, read_size);
                        continue;
                    }
                    if(!fs::WriteFile(entry_path, read_buf, read_buf_size, true)) {
                        UL_LOG_WARN("Unable to save theme zip file index %d (from %d total) to '%s'...", i, file_count, entry_path.c_str());
                    }
                    free(read_buf);
                }
                zip_entry_close(zip_file);
            }
        }
        zip_close(zip_file);
    }

    void EnsureCacheActiveTheme(const Config &cfg) {
        const auto manifest_path = fs::JoinPath(ActiveThemeCachePath, ThemeManifestPath);
        // This should be enough to check whether the extracted active theme was removed
        if(!fs::ExistsFile(manifest_path)) {
            CacheActiveTheme(cfg);
        }
    }

    void RemoveActiveThemeCache() {
        fs::CleanDirectory(ActiveThemeCachePath);
    }

    void LoadLanguageJsons(const std::string &lang_base, util::JSON &lang, util::JSON &def) {
        const auto default_lang_file_path = fs::JoinPath(BuiltinMenuLanguagesPath, DefaultLanguage) + ".json";
        UL_RC_ASSERT(ul::util::LoadJSONFromFile(def, default_lang_file_path));
        
        u64 lang_code = 0;
        UL_RC_ASSERT(setGetLanguageCode(&lang_code));
        auto sys_lang = reinterpret_cast<char*>(&lang_code);

        #define _TRY_LOAD_LANG(lang_str) { \
            const auto ext_path = fs::JoinPath(lang_base, lang_str) + ".json"; \
            if(R_SUCCEEDED(util::LoadJSONFromFile(lang, ext_path))) { \
                return; \
            } \
            const auto builtin_path = fs::JoinPath(BuiltinMenuLanguagesPath, lang_str) + ".json"; \
            if(R_SUCCEEDED(util::LoadJSONFromFile(lang, builtin_path))) { \
                return; \
            } \
        }

        _TRY_LOAD_LANG(sys_lang);

        // Special cases for languages that may be covered by a unifying translation
        if((strcmp(sys_lang, "en-US") == 0) || (strcmp(sys_lang, "en-GB") == 0)) {
            _TRY_LOAD_LANG("en");
        }
        if(strcmp(sys_lang, "fr-CA") == 0) {
            _TRY_LOAD_LANG("fr");
        }
        if(strcmp(sys_lang, "es-419") == 0) {
            _TRY_LOAD_LANG("es");
        }
        if(strcmp(sys_lang, "pt-BR") == 0) {
            _TRY_LOAD_LANG("pt");
        }

        lang = def;
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
