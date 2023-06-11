#include <ul/cfg/cfg_Config.hpp>
#include <ul/util/util_String.hpp>
#include <ul/util/util_Stl.hpp>
#include <ul/os/os_Titles.hpp>

namespace ul::cfg {

    namespace {

        void DoCacheHomebrew(const std::string &nro_path) {
            const auto cache_nro_icon_path = GetNroCacheIconPath(nro_path);
            auto f = fopen(nro_path.c_str(), "rb");
            if(f) {
                if(fseek(f, sizeof(NroStart), SEEK_SET) == 0) {
                    NroHeader header = {};
                    if(fread(&header, sizeof(header), 1, f) == 1) {
                        if(fseek(f, header.size, SEEK_SET) == 0) {
                            NroAssetHeader asset_header = {};
                            if(fread(&asset_header, sizeof(asset_header), 1, f) == 1) {
                                if(asset_header.magic == NROASSETHEADER_MAGIC) {
                                    if((asset_header.icon.offset > 0) && (asset_header.icon.size > 0)) {
                                        auto icon_buf = new u8[asset_header.icon.size]();
                                        if(fseek(f, header.size + asset_header.icon.offset, SEEK_SET) == 0) {
                                            if(fread(icon_buf, asset_header.icon.size, 1, f) == 1) {
                                                fs::WriteFile(cache_nro_icon_path, icon_buf, asset_header.icon.size, true);
                                            }
                                        }
                                        delete[] icon_buf;
                                    }
                                }
                            }
                        }
                    }
                }
                fclose(f);
            }
        }

        void CacheHomebrew(const std::string &hb_base_path) {
            UL_FS_FOR(hb_base_path, name, path, is_dir, is_file, {
                if(dt->d_type & DT_DIR) {
                    CacheHomebrew(path);
                }
                else if(util::StringEndsWith(name, ".nro")) {
                    DoCacheHomebrew(path);
                }
            });
        }

        void CacheInstalledTitles() {
            const auto titles = os::QueryInstalledTitles();

            auto control_data = new NsApplicationControlData();
            for(const auto &title: titles) {
                const auto cache_icon_path = cfg::GetTitleCacheIconPath(title.app_id);
                if(R_SUCCEEDED(nsGetApplicationControlData(NsApplicationControlSource_Storage, title.app_id, control_data, sizeof(NsApplicationControlData), nullptr))) {
                    fs::WriteFile(cache_icon_path, control_data->icon, sizeof(control_data->icon), true);
                }
            }
            delete control_data;
        }

        void ProcessStringsFromNacp(RecordStrings &strs, NacpStruct *nacp) {
            NacpLanguageEntry *lang_entry = nullptr;
            nacpGetLanguageEntry(nacp, &lang_entry);
            if(lang_entry == nullptr) {
                for(u32 i = 0; i < 16; i++) {
                    lang_entry = &nacp->lang[i];
                    if((strlen(lang_entry->name) > 0) && (strlen(lang_entry->author) > 0)) {
                        break;
                    }
                    lang_entry = nullptr;
                }
            }

            if(lang_entry != nullptr) {
                strs.name = lang_entry->name;
                strs.author = lang_entry->author;
                strs.version = nacp->display_version;
            }
        }

    }

    std::vector<TitleRecord> QueryAllHomebrew(const std::string &base) {
        std::vector<TitleRecord> nros;
        UL_FS_FOR(base, name, path, is_dir, is_file, {
            if(dt->d_type & DT_DIR) {
                const auto hb = QueryAllHomebrew(path);
                if(!hb.empty()) {
                    nros.insert(nros.begin(), hb.begin(), hb.end());
                }
            }
            else {
                if(util::StringEndsWith(name, ".nro")) {
                    auto f = fopen(path.c_str(), "rb");
                    if(f) {
                        fseek(f, 0, SEEK_END);
                        const auto f_size = ftell(f);
                        rewind(f);

                        if(fseek(f, sizeof(NroStart), SEEK_SET) == 0) {
                            NroHeader header;
                            if(fread(&header, sizeof(header), 1, f) == 1) {
                                if((header.magic == NROHEADER_MAGIC) && (f_size >= header.size)) {
                                    TitleRecord rec = {};
                                    rec.title_type = TitleType::Homebrew;
                                    strcpy(rec.nro_target.nro_path, path.c_str());
                                    nros.push_back(rec);
                                }
                            }
                        }
                        fclose(f);
                    }
                }
            }
        });
        return nros;
    }

    void CacheEverything(const std::string &hb_base_path) {
        fs::CleanDirectory(TitleCachePath);
        CacheInstalledTitles();

        fs::CleanDirectory(EntryCachePath);
        CacheHomebrew(hb_base_path);
    }

    std::string GetRecordIconPath(const TitleRecord &record) {
        auto icon_path = record.icon;
        if(icon_path.empty()) {
            if(record.title_type == TitleType::Homebrew) {
                icon_path = GetNroCacheIconPath(record.nro_target.nro_path);
                if(!fs::ExistsFile(icon_path)) {
                    // NRO has no icon, use the default one
                    icon_path = DefaultHomebrewIconPath;
                }
            }
            else if(record.title_type == TitleType::Installed) {
                icon_path = GetTitleCacheIconPath(record.app_id);
            }
        }
        return icon_path;
    }

    std::string GetRecordJsonPath(const TitleRecord &record) {
        auto json_name = record.json_name;
        if(json_name.empty()) {
            if(record.title_type == TitleType::Homebrew) {
                json_name = std::to_string(fs::GetFileSize(record.nro_target.nro_path)) + ".json";
            }
            else if(record.title_type == TitleType::Installed) {
                const auto app_id_str = util::FormatProgramId(record.app_id);
                json_name = app_id_str + ".json";
            }
        }
        return JoinPath(EntriesPath, json_name);
    }

    RecordInformation GetRecordInformation(const TitleRecord &record) {
        RecordInformation info = {};
        info.icon_path = GetRecordIconPath(record);
        if(record.title_type == TitleType::Homebrew) {
            auto f = fopen(record.nro_target.nro_path, "rb");
            if(f) {
                fseek(f, sizeof(NroStart), SEEK_SET);
                NroHeader hdr = {};
                if(fread(&hdr, 1, sizeof(NroHeader), f) == sizeof(NroHeader)) {
                    fseek(f, hdr.size, SEEK_SET);
                    NroAssetHeader ahdr = {};
                    if(fread(&ahdr, 1, sizeof(NroAssetHeader), f) == sizeof(NroAssetHeader)) {
                        if(ahdr.magic == NROASSETHEADER_MAGIC) {
                            if(ahdr.nacp.size > 0) {
                                NacpStruct nacp = {};
                                fseek(f, hdr.size + ahdr.nacp.offset, SEEK_SET);
                                fread(&nacp, 1, ahdr.nacp.size, f);
                                ProcessStringsFromNacp(info.strings, &nacp);
                            }
                        }
                    }
                }
                fclose(f);
            }
        }
        else {
            auto control_data = new NsApplicationControlData();
            nsGetApplicationControlData(NsApplicationControlSource_Storage, record.app_id, control_data, sizeof(NsApplicationControlData), nullptr);
            ProcessStringsFromNacp(info.strings, &control_data->nacp);
            delete control_data;
        }
        if(!record.name.empty()) {
            info.strings.name = record.name;
        }
        if(!record.author.empty()) {
            info.strings.author = record.author;
        }
        if(!record.version.empty()) {
            info.strings.version = record.version;
        }
        return info;
    }

    Theme LoadTheme(const std::string &base_name) {
        Theme theme = {
            .base_name = base_name
        };
        auto theme_dir = JoinPath(ThemesPath, base_name);
        auto manifest_path = JoinPath(theme_dir, "theme/Manifest.json");
        if(base_name.empty() || !fs::ExistsFile(manifest_path)) {
            theme_dir = DefaultThemePath;
        }
        manifest_path = JoinPath(theme_dir, "theme/Manifest.json");

        util::JSON manifest_json;
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
        auto base_res = JoinPath(base.path, resource_base);
        if(fs::ExistsFile(base_res)) {
            return base_res;
        }

        base_res = JoinPath(DefaultThemePath, resource_base);
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

    void SaveRecord(const TitleRecord &record) {
        auto entry = util::JSON::object();
        entry["type"] = static_cast<u32>(record.title_type);
        entry["folder"] = record.sub_folder;

        if(!record.name.empty()) {
            entry["name"] = record.name;
        }
        if(!record.author.empty()) {
            entry["author"] = record.author;
        }
        if(!record.version.empty()) {
            entry["version"] = record.version;
        }
        if(!record.icon.empty()) {
            entry["icon"] = record.icon;
        }

        if(record.title_type == TitleType::Homebrew) {
            entry["nro_path"] = record.nro_target.nro_path;
            if(strlen(record.nro_target.nro_argv)) {
                if(strcasecmp(record.nro_target.nro_path, record.nro_target.nro_argv) != 0) {
                    entry["nro_argv"] = record.nro_target.nro_argv;
                }
            }
        }
        else if(record.title_type == TitleType::Installed) {
            entry["application_id"] = util::FormatProgramId(record.app_id);
        }
        
        const auto json_path = GetRecordJsonPath(record);
        if(fs::ExistsFile(json_path)) {
            fs::DeleteFile(json_path);
        }

        // TODO: check res?
        util::SaveJSON(json_path, entry);
    }

    bool MoveRecordTo(TitleList &list, const TitleRecord &record, const std::string &folder_name) {
        bool title_found = false;
        TitleRecord record_copy = {};
        std::string record_json_name;

        // Search in root first
        const auto find_in_root = UL_STL_FIND_IF(list.root.titles, title_item, record.Equals(title_item));
        if(UL_STL_FOUND(list.root.titles, find_in_root)) {
            // It is already on root...?
            if(folder_name.empty()) {
                return true;
            }
            record_json_name = UL_STL_UNWRAP(find_in_root).json_name;

            list.root.titles.erase(find_in_root);
            title_found = true;
        }

        // If not found yet, search on all dirs if the title is present
        if(!title_found) {
            for(auto &folder: list.folders) {
                const auto find_in_folder = UL_STL_FIND_IF(folder.titles, title_item, record.Equals(title_item));
                if(UL_STL_FOUND(folder.titles, find_in_folder)) {
                    // It is already on that folder...?
                    if(folder.name == folder_name) {
                        return true;
                    }
                    record_json_name = UL_STL_UNWRAP(find_in_folder).json_name;

                    folder.titles.erase(find_in_folder);
                    title_found = true;
                    break;
                }
            }
        }

        if(title_found) {
            TitleRecord title = record;
            title.json_name = record_json_name;
            title.sub_folder = folder_name;

            // Add (move) it to root again
            if(folder_name.empty()) {
                list.root.titles.push_back(title);
            }
            // Add it to the new folder
            else {
                const auto find_folder = UL_STL_FIND_IF(list.folders, folder_item, (folder_item.name == folder_name));
                if(UL_STL_FOUND(list.folders, find_folder)) {
                    UL_STL_UNWRAP(find_folder).titles.push_back(title);
                }
                else {
                    TitleFolder folder = {};
                    folder.name = folder_name;
                    folder.titles.push_back(title);
                    list.folders.push_back(folder);
                }
            }

            SaveRecord(title);
        }

        return title_found;
    }

    TitleFolder &FindFolderByName(TitleList &list, const std::string &name) {
        if(!name.empty()) {
            auto f = UL_STL_FIND_IF(list.folders, fld, (fld.name == name));
            if(UL_STL_FOUND(list.folders, f)) {
                return UL_STL_UNWRAP(f);
            }
        }
        return list.root;
    }

    void RenameFolder(TitleList &list, const std::string &old_name, const std::string &new_name) {
        auto &folder = FindFolderByName(list, old_name);
        if(!folder.name.empty()) {
            folder.name = new_name;
            for(auto &entry: folder.titles) {
                entry.sub_folder = new_name;
                SaveRecord(entry);
            }
        }
    }

    bool ExistsRecord(const TitleList &list, const TitleRecord &record) {
        auto title_found = false;
        TitleRecord record_copy = {};
        std::string record_json_name;

        // Search in root first
        const auto find_in_root = UL_STL_FIND_IF(list.root.titles, title_item, record.Equals(title_item));
        if(UL_STL_FOUND(list.root.titles, find_in_root)) {
            if(!UL_STL_UNWRAP(find_in_root).json_name.empty()) {
                title_found = true;
            }
        }

        // If not found yet, search on all dirs if the title is present
        if(!title_found) {
            for(auto &folder: list.folders) {
                const auto find_in_folder = UL_STL_FIND_IF(folder.titles, title_item, record.Equals(title_item));
                if(UL_STL_FOUND(folder.titles, find_in_folder)) {
                    if(!UL_STL_UNWRAP(find_in_folder).json_name.empty()) {
                        title_found = true;
                        break;
                    }
                }
            }
        }

        return title_found;
    }

    TitleList LoadTitleList() {
        TitleList list = {};
        
        // Installed titles first
        auto titles = os::QueryInstalledTitles();

        UL_FS_FOR("sdmc:/ulaunch/entries", name, path, is_dir, is_file, {
            util::JSON entry;
            const auto rc = util::LoadJSONFromFile(entry, path);
            if(R_SUCCEEDED(rc)) {
                const auto type = static_cast<TitleType>(entry.value("type", static_cast<u32>(TitleType::Invalid)));
                if(type == TitleType::Installed) {
                    const std::string app_id_str = entry.value("application_id", "");
                    if(!app_id_str.empty()) {
                        const std::string folder = entry.value("folder", "");
                        const auto app_id = util::Get64FromString(app_id_str);
                        if(app_id > 0) {
                            if(!folder.empty()) {
                                TitleRecord rec = {
                                    .json_name = name,
                                    .title_type = type,
                                    .sub_folder = folder,
                                    .icon = entry.value("icon", ""),
                                    .app_id = app_id,
                                    .name = entry.value("name", ""),
                                    .author = entry.value("author", ""),
                                    .version = entry.value("version", "")
                                };

                                const auto find_title = UL_STL_FIND_IF(titles, title_item, title_item.app_id == app_id);
                                if(UL_STL_FOUND(titles, find_title)) {
                                    titles.erase(find_title);
                                }

                                const auto find_folder = UL_STL_FIND_IF(list.folders, folder_item, folder_item.name == folder);
                                if(UL_STL_FOUND(list.folders, find_folder)) {
                                    UL_STL_UNWRAP(find_folder).titles.push_back(rec);
                                }
                                else {
                                    TitleFolder title_folder = {
                                        .name = folder
                                    };
                                    title_folder.titles.push_back(rec);

                                    list.folders.push_back(title_folder);
                                }
                            }
                        }
                    }
                }
                else if(type == TitleType::Homebrew) {
                    const std::string nro_path = entry.value("nro_path", "");
                    if(fs::ExistsFile(nro_path)) {
                        const std::string folder = entry.value("folder", "");
                        TitleRecord rec = {
                            .json_name = name,
                            .title_type = type,
                            .sub_folder = folder,
                            .icon = entry.value("icon", ""),
                            .name = entry.value("name", ""),
                            .author = entry.value("author", ""),
                            .version = entry.value("version", "")
                        };
                        const std::string argv = entry.value("nro_argv", "");
                        strcpy(rec.nro_target.nro_path, nro_path.c_str());
                        if(!argv.empty()) {
                            strcpy(rec.nro_target.nro_argv, argv.c_str());
                        }
                        
                        if(folder.empty()) {
                            list.root.titles.push_back(rec);
                        }
                        else {
                            const auto find_folder = UL_STL_FIND_IF(list.folders, folder_item, folder_item.name == folder);
                            if(UL_STL_FOUND(list.folders, find_folder)) {
                                UL_STL_UNWRAP(find_folder).titles.push_back(rec);
                            }
                            else {
                                TitleFolder title_folder = {
                                    .name = folder
                                };
                                title_folder.titles.push_back(rec);

                                list.folders.push_back(title_folder);
                            }
                        }
                    }
                }
            }
        });

        // Add cached icons to non-custom title entries
        for(auto &title: titles) {
            title.icon = cfg::GetTitleCacheIconPath(title.app_id);
        }

        list.root.titles.insert(list.root.titles.end(), titles.begin(), titles.end());
        return list;
    }

    std::string GetNroCacheIconPath(const std::string &path) {
        char path_copy[FS_MAX_PATH] = {};
        strcpy(path_copy, path.c_str());
        u8 hash[0x20] = {0};
        sha256CalculateHash(hash, path_copy, FS_MAX_PATH);

        std::stringstream strm;
        strm << "sdmc:/ulaunch/nro/";
        // Use the first half of the hash, like N does with NCAs.
        for(u32 i = 0; i < sizeof(hash) / 2; i++) {
            strm << std::setw(2) << std::setfill('0') << std::hex << std::nouppercase << static_cast<u32>(hash[i]);
        }
        strm << ".jpg";
        return strm.str();
    }

}