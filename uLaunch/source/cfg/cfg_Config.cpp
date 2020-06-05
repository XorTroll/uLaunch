#include <cfg/cfg_Config.hpp>
#include <os/os_Titles.hpp>
#include <util/util_Misc.hpp>
#include <util/util_String.hpp>
#include <fs/fs_Stdio.hpp>
#include <db/db_Save.hpp>
#include <util/util_Convert.hpp>

namespace cfg {

    static void CacheHomebrew(const std::string &nro_path) {
        auto nroimg = GetNROCacheIconPath(nro_path);
        auto f = fopen(nro_path.c_str(), "rb");
        if(f) {
            fseek(f, sizeof(NroStart), SEEK_SET);
            NroHeader hdr = {};
            if(fread(&hdr, sizeof(NroHeader), 1, f) == 1) {
                fseek(f, hdr.size, SEEK_SET);
                NroAssetHeader ahdr = {};
                if(fread(&ahdr, sizeof(NroAssetHeader), 1, f) == 1) {
                    if(ahdr.magic == NROASSETHEADER_MAGIC) {
                        if((ahdr.icon.offset > 0) && (ahdr.icon.size > 0)) {
                            auto iconbuf = new u8[ahdr.icon.size]();
                            fseek(f, hdr.size + ahdr.icon.offset, SEEK_SET);
                            fread(iconbuf, ahdr.icon.size, 1, f);
                            fs::WriteFile(nroimg, iconbuf, ahdr.icon.size, true);
                            delete[] iconbuf;
                        }
                    }
                }
            }
            fclose(f);
        }
    }

    static void CacheInstalledTitles() {
        UL_OS_FOR_EACH_APP_RECORD(rec, {
            NsApplicationControlData control = {};
            rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, rec.application_id, &control, sizeof(control), nullptr);
            if(R_SUCCEEDED(rc)) {
                auto fname = cfg::GetTitleCacheIconPath(rec.application_id);
                fs::WriteFile(fname, control.icon, sizeof(control.icon), true);
            }
        });
    }

    static void CacheAllHomebrew(const std::string &hb_base_path) {
        UL_FS_FOR(hb_base_path, name, path, {
            if(dt->d_type & DT_DIR) {
                CacheAllHomebrew(path);
            }
            else if(util::StringEndsWith(name, ".nro")) {
                CacheHomebrew(path);
            }
        });
    }

    static void ProcessStringsFromNACP(RecordStrings &strs, NacpStruct *nacp) {
        NacpLanguageEntry *lent = nullptr;
        nacpGetLanguageEntry(nacp, &lent);
        if(lent == nullptr) {
            for(u32 i = 0; i < 16; i++) {
                lent = &nacp->lang[i];
                if((strlen(lent->name) > 0) && (strlen(lent->author) > 0)) {
                    break;
                }
                lent = nullptr;
            }
        }
        if(lent != nullptr) {
            strs.name = lent->name;
            strs.author = lent->author;
            strs.version = nacp->display_version;
        }
    }

    std::vector<TitleRecord> QueryAllHomebrew(const std::string &base) {
        std::vector<TitleRecord> nros;
        UL_FS_FOR(base, name, path, {
            if(dt->d_type & DT_DIR) {
                auto hb = QueryAllHomebrew(path);
                if(!hb.empty()) {
                    nros.insert(nros.begin(), hb.begin(), hb.end());
                }
            }
            else {
                if(util::StringEndsWith(name, ".nro")) {
                    TitleRecord rec = {};
                    rec.title_type = (u32)TitleType::Homebrew;
                    strcpy(rec.nro_target.nro_path, path.c_str());
                    nros.push_back(rec);
                }
            }
        });
        return nros;
    }

    void CacheEverything(const std::string &hb_base_path) {
        CacheInstalledTitles();
        CacheAllHomebrew(hb_base_path);
    }

    std::string GetRecordIconPath(TitleRecord record) {
        std::string icon = record.icon;
        if(icon.empty()) {
            const auto type = static_cast<TitleType>(record.title_type);
            if(type == TitleType::Homebrew) {
                icon = GetNROCacheIconPath(record.nro_target.nro_path);
            }
            else if(type == TitleType::Installed) {
                icon = GetTitleCacheIconPath(record.app_id);
            }
        }
        return icon;
    }

    RecordInformation GetRecordInformation(TitleRecord record) {
        RecordInformation info = {};
        info.icon_path = GetRecordIconPath(record);
        const auto type = static_cast<TitleType>(record.title_type);
        if(type == TitleType::Homebrew) {
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
                                ProcessStringsFromNACP(info.strings, &nacp);
                            }
                        }
                    }
                }
                fclose(f);
            }
        }
        else {
            NsApplicationControlData cdata = {};
            nsGetApplicationControlData(NsApplicationControlSource_Storage, record.app_id, &cdata, sizeof(cdata), nullptr);
            ProcessStringsFromNACP(info.strings, &cdata.nacp);
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
        Theme theme = {};
        theme.base_name = base_name;
        auto themedir = std::string(UL_THEMES_PATH) + "/" + base_name;
        auto metajson = themedir + "/theme/Manifest.json";
        if(base_name.empty() || !fs::ExistsFile(metajson)) {
            themedir = CFG_THEME_DEFAULT;
        }
        metajson = themedir + "/theme/Manifest.json";
        JSON meta;
        auto rc = util::LoadJSONFromFile(meta, metajson);
        if(R_SUCCEEDED(rc)) {
            theme.manifest.name = meta.value("name", "'" + base_name + "'");
            theme.manifest.format_version = meta.value("format_version", 0);
            theme.manifest.release = meta.value("release", "");
            theme.manifest.description = meta.value("description", "");
            theme.manifest.author = meta.value("author", "");
            theme.path = themedir;
            return theme;
        }
        return LoadTheme("");
    }

    std::vector<Theme> LoadThemes() {
        std::vector<Theme> themes;
        UL_FS_FOR(std::string(UL_THEMES_PATH), name, path, {
            auto theme = LoadTheme(name);
            if(!theme.path.empty()) {
                themes.push_back(theme);
            }
        });
        return themes;
    }

    std::string GetAssetByTheme(const Theme &base, const std::string &resource_base) {
        auto base_res = base.path + "/" + resource_base;
        if(fs::ExistsFile(base_res)) {
            return base_res;
        }
        base_res = std::string(CFG_THEME_DEFAULT) + "/" + resource_base;
        if(fs::ExistsFile(base_res)) {
            return base_res;
        }
        return "";
    }

    std::string GetLanguageString(const JSON &lang, const JSON &def, const std::string &name) {
        auto str = lang.value(name, "");
        if(str.empty()) {
            str = def.value(name, "");
        }
        return str;
    }

    Config CreateNewAndLoadConfig() {
        // Default constructor sets everything
        Config cfg = {};
        SaveConfig(cfg);
        return cfg;
    }

    Config LoadConfig()
    {
        // Default constructor sets everything
        Config cfg = {};
        JSON cfgjson;
        auto rc = util::LoadJSONFromFile(cfgjson, CFG_CONFIG_JSON);
        if(R_SUCCEEDED(rc)) {
            cfg.theme_name = cfgjson.value("theme_name", "");
            cfg.system_title_override_enabled = cfgjson.value("system_title_override_enabled", false);
            cfg.viewer_usb_enabled = cfgjson.value("viewer_usb_enabled", false);
            auto menu_id_str = cfgjson.value("menu_program_id", "");
            if(!menu_id_str.empty()) {
                cfg.menu_program_id = util::Get64FromString(menu_id_str);
            }
            auto hb_applet_id_str = cfgjson.value("homebrew_applet_program_id", "");
            if(!hb_applet_id_str.empty()) {
                cfg.homebrew_applet_program_id = util::Get64FromString(hb_applet_id_str);
            }
            auto hb_title_id_str = cfgjson.value("homebrew_title_application_id", "");
            if(!hb_title_id_str.empty()) {
                cfg.homebrew_title_application_id = util::Get64FromString(hb_title_id_str);
            }
            // Doing this saves any fields not set previously
            SaveConfig(cfg);
            return cfg;
        }
        fs::DeleteFile(CFG_CONFIG_JSON);
        return CreateNewAndLoadConfig();
    }

    void SaveConfig(const Config &cfg)
    {
        fs::DeleteFile(CFG_CONFIG_JSON);
        auto json = JSON::object();
        json["theme_name"] = cfg.theme_name;
        json["viewer_usb_enabled"] = cfg.viewer_usb_enabled;
        json["system_title_override_enabled"] = cfg.system_title_override_enabled;
        json["menu_program_id"] = util::FormatApplicationId(cfg.menu_program_id);
        json["homebrew_applet_program_id"] = util::FormatApplicationId(cfg.homebrew_applet_program_id);
        json["homebrew_title_application_id"] = util::FormatApplicationId(cfg.homebrew_title_application_id);
        std::ofstream ofs(CFG_CONFIG_JSON);
        ofs << std::setw(4) << json;
        ofs.close();
    }

    void SaveRecord(TitleRecord &record)
    {
        auto entry = JSON::object();
        entry["type"] = record.title_type;
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

        // Prepare JSON path
        std::string basepath = UL_ENTRIES_PATH;
        std::string json = basepath;
        const auto type = static_cast<TitleType>(record.title_type);
        if(type == TitleType::Homebrew) {
            auto jsonname = std::to_string(fs::GetFileSize(record.nro_target.nro_path)) + ".json";
            if(record.json_name.empty()) {
                record.json_name = jsonname;
            }
            json += "/" + jsonname;
            entry["nro_path"] = record.nro_target.nro_path;
            if(strlen(record.nro_target.nro_argv)) {
                if(strcasecmp(record.nro_target.nro_path, record.nro_target.nro_argv) != 0) {
                    entry["nro_argv"] = record.nro_target.nro_argv;
                }
            }
        }
        else if(type == TitleType::Installed) {
            auto strappid = util::FormatApplicationId(record.app_id);
            auto jsonname = strappid + ".json";
            if(record.json_name.empty()) {
                record.json_name = jsonname;
            }
            json += "/" + jsonname;
            entry["application_id"] = strappid;
        }
        if(!record.json_name.empty()) {
            json = basepath + "/" + record.json_name;
        }
        if(fs::ExistsFile(json)) {
            fs::DeleteFile(json);
        }

        std::ofstream ofs(json);
        ofs << std::setw(4) << entry;
        ofs.close();
    }

    void RemoveRecord(TitleRecord &record) {
        // Prepare JSON path
        std::string basepath = UL_ENTRIES_PATH;
        std::string json = basepath;
        const auto type = static_cast<TitleType>(record.title_type);
        if(type == TitleType::Homebrew) {
            auto jsonname = std::to_string(fs::GetFileSize(record.nro_target.nro_path)) + ".json";
            if(record.json_name.empty()) {
                record.json_name = jsonname;
            }
            json += "/" + jsonname;
        }
        else if(type == TitleType::Installed) {
            auto strappid = util::FormatApplicationId(record.app_id);
            auto jsonname = strappid + ".json";
            if(record.json_name.empty()) {
                record.json_name = jsonname;
            }
            json += "/" + jsonname;
        }
        if(!record.json_name.empty()) {
            json = basepath + "/" + record.json_name;
        }
        fs::DeleteFile(json);
    }

    bool MoveTitleToDirectory(TitleList &list, u64 app_id, const std::string &folder) {
        bool title_found = false;
        TitleRecord record_copy = {};
        std::string recjson;

        // Search in root first
        auto find = STL_FIND_IF(list.root.titles, tit, (tit.app_id == app_id));
        if(STL_FOUND(list.root.titles, find)) {
            if(folder.empty()) return true;  // It is already on root...?
            recjson = STL_UNWRAP(find).json_name;

            list.root.titles.erase(find);
            title_found = true;
        }

        // If not found yet, search on all dirs if the title is present
        if(!title_found) {
            for(auto &fld: list.folders) {
                auto find = STL_FIND_IF(fld.titles, entry, (entry.app_id == app_id));
                if(STL_FOUND(fld.titles, find)) {
                    // It is already on that folder...?
                    if(fld.name == folder) {
                        return true;
                    }
                    recjson = STL_UNWRAP(find).json_name;

                    fld.titles.erase(find);
                    title_found = true;
                    break;
                }
            }
        }

        if(title_found) {
            TitleRecord title = {};
            title.app_id = app_id;
            title.title_type = (u32)TitleType::Installed;
            title.json_name = recjson;
            title.sub_folder = folder;

            // Add (move) it to root again
            if(folder.empty()) {
                list.root.titles.push_back(title);
            }
            // Add it to the new folder
            else {
                auto find2 = STL_FIND_IF(list.folders, fld, (fld.name == folder));
                if(STL_FOUND(list.folders, find2)) {
                    STL_UNWRAP(find2).titles.push_back(title);
                }
                else {
                    TitleFolder fld = {};
                    fld.name = folder;
                    fld.titles.push_back(title);
                    list.folders.push_back(fld);
                }
            }

            SaveRecord(title);
        }

        return title_found;
    }

    bool MoveRecordTo(TitleList &list, TitleRecord record, const std::string &folder) {
        bool title_found = false;
        TitleRecord record_copy = {};
        std::string recjson;
        const auto type = static_cast<TitleType>(record.title_type);

        // Search in root first
        if(type == TitleType::Installed) {
            auto find = STL_FIND_IF(list.root.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
            if(STL_FOUND(list.root.titles, find)) {
                // It is already on root...?
                if(folder.empty()) {
                    return true;
                }
                recjson = STL_UNWRAP(find).json_name;

                list.root.titles.erase(find);
                title_found = true;
            }
        }
        else {
            auto find = STL_FIND_IF(list.root.titles, tit, (tit.title_type == record.title_type) && (tit.nro_target.nro_path == record.nro_target.nro_path));
            if(STL_FOUND(list.root.titles, find)) {
                // It is already on root...?
                if(folder.empty()) {
                    return true;
                }
                recjson = STL_UNWRAP(find).json_name;

                list.root.titles.erase(find);
                title_found = true;
            }
        }

        // If not found yet, search on all dirs if the title is present
        if(!title_found) {
            for(auto &fld: list.folders) {
                if(type == TitleType::Installed) {
                    auto find = STL_FIND_IF(fld.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
                    if(STL_FOUND(fld.titles, find)) {
                        // It is already on that folder...?
                        if(fld.name == folder) {
                            return true;
                        }
                        recjson = STL_UNWRAP(find).json_name;

                        fld.titles.erase(find);
                        title_found = true;
                        break;
                    }
                }
                else {
                    auto find = STL_FIND_IF(fld.titles, tit, (tit.title_type == record.title_type) && (tit.nro_target.nro_path == record.nro_target.nro_path));
                    if(STL_FOUND(fld.titles, find)) {
                        // It is already on that folder...?
                        if(fld.name == folder) {
                            return true;
                        }
                        recjson = STL_UNWRAP(find).json_name;

                        fld.titles.erase(find);
                        title_found = true;
                        break;
                    }
                }
            }
        }

        if(title_found) {
            TitleRecord title = record;
            title.json_name = recjson;
            title.sub_folder = folder;

            // Add (move) it to root again
            if(folder.empty()) {
                list.root.titles.push_back(title);
            }
            // Add it to the new folder
            else {
                auto find2 = STL_FIND_IF(list.folders, fld, (fld.name == folder));
                if(STL_FOUND(list.folders, find2)) {
                    STL_UNWRAP(find2).titles.push_back(title);
                }
                else {
                    TitleFolder fld = {};
                    fld.name = folder;
                    fld.titles.push_back(title);
                    list.folders.push_back(fld);
                }
            }

            SaveRecord(title);
        }

        return title_found;
    }

    TitleFolder &FindFolderByName(TitleList &list, const std::string &name)
    {
        if(!name.empty()) {
            auto f = STL_FIND_IF(list.folders, fld, (fld.name == name));
            if(STL_FOUND(list.folders, f)) {
                return STL_UNWRAP(f);
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

    bool ExistsRecord(TitleList &list, TitleRecord record)
    {
        bool title_found = false;
        TitleRecord record_copy = {};
        std::string recjson;
        const auto type = static_cast<TitleType>(record.title_type);

        // Search in root first
        if(type == TitleType::Installed) {
            auto find = STL_FIND_IF(list.root.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
            if(STL_FOUND(list.root.titles, find)) {
                if(!STL_UNWRAP(find).json_name.empty()) {
                    title_found = true;
                }
            }
        }
        else {
            auto find = STL_FIND_IF(list.root.titles, tit, (tit.title_type == record.title_type) && (tit.nro_target.nro_path == record.nro_target.nro_path));
            if(STL_FOUND(list.root.titles, find)) {
                if(!STL_UNWRAP(find).json_name.empty()) {
                    title_found = true;
                }
            }
        }

        // If not found yet, search on all dirs if the title is present
        if(!title_found) {
            for(auto &fld: list.folders) {
                if(type == TitleType::Installed) {
                    auto find = STL_FIND_IF(fld.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
                    if(STL_FOUND(fld.titles, find)) {
                        if(!STL_UNWRAP(find).json_name.empty()) {
                            title_found = true;
                            break;
                        }
                    }
                }
                else {
                    auto find = STL_FIND_IF(fld.titles, tit, (tit.title_type == record.title_type) && (tit.nro_target.nro_path == record.nro_target.nro_path));
                    if(STL_FOUND(fld.titles, find)) {
                        if(!STL_UNWRAP(find).json_name.empty()) {
                            title_found = true;
                            break;
                        }
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

        UL_FS_FOR(std::string(UL_ENTRIES_PATH), name, path, {
            JSON entry;
            auto rc = util::LoadJSONFromFile(entry, path);
            if(R_SUCCEEDED(rc)) {
                auto type = static_cast<TitleType>(entry.value("type", 0));
                if(type == TitleType::Installed) {
                    std::string appidstr = entry.value("application_id", "");
                    if(!appidstr.empty()) {
                        std::string folder = entry.value("folder", "");
                        auto appid = util::Get64FromString(appidstr);
                        if(appid > 0) {
                            if(!folder.empty()) {
                                TitleRecord rec = {};
                                rec.json_name = name;
                                rec.app_id = appid;
                                rec.title_type = static_cast<u32>(TitleType::Installed);
                                rec.name = entry.value("name", "");
                                rec.author = entry.value("author", "");
                                rec.version = entry.value("version", "");
                                rec.icon = entry.value("icon", "");

                                auto find = STL_FIND_IF(titles, tit, (tit.app_id == appid));
                                if(STL_FOUND(titles, find)) {
                                    titles.erase(find);
                                }

                                auto find2 = STL_FIND_IF(list.folders, fld, (fld.name == folder));
                                if(STL_FOUND(list.folders, find2)) {
                                    STL_UNWRAP(find2).titles.push_back(rec);
                                }
                                else {
                                    TitleFolder fld = {};
                                    fld.name = folder;
                                    fld.titles.push_back(rec);
                                    list.folders.push_back(fld);
                                }
                            }
                        }
                    }
                }
                else if(type == TitleType::Homebrew) {
                    std::string nropath = entry.value("nro_path", "");
                    if(fs::ExistsFile(nropath)) {
                        TitleRecord rec = {};
                        rec.json_name = name;
                        rec.title_type = (u32)type;
                        rec.name = entry.value("name", "");
                        rec.author = entry.value("author", "");
                        rec.version = entry.value("version", "");
                        
                        std::string argv = entry.value("nro_argv", "");
                        strcpy(rec.nro_target.nro_path, nropath.c_str());
                        if(!argv.empty()) {
                            strcpy(rec.nro_target.nro_argv, argv.c_str());
                        }
                        std::string folder = entry.value("folder", "");
                        rec.sub_folder = folder;
                        rec.icon = entry.value("icon", "");
                        
                        if(folder.empty()) {
                            list.root.titles.push_back(rec);
                        }
                        else {
                            auto find = STL_FIND_IF(list.folders, fld, (fld.name == folder));
                            if(STL_FOUND(list.folders, find)) {
                                STL_UNWRAP(find).titles.push_back(rec);
                            }
                            else {
                                TitleFolder fld = {};
                                fld.name = folder;
                                fld.titles.push_back(rec);
                                list.folders.push_back(fld);
                            }
                        }
                    }
                }
            }
        });

        list.root.titles.insert(list.root.titles.end(), titles.begin(), titles.end());
        return list;
    }

    std::string GetTitleCacheIconPath(u64 app_id) {
        auto strappid = util::FormatApplicationId(app_id);
        return UL_BASE_SD_DIR "/title/" + strappid + ".jpg";
    }

    std::string GetNROCacheIconPath(const std::string &path) {
        char pathcopy[FS_MAX_PATH] = {0};
        strcpy(pathcopy, path.c_str());
        u8 hash[0x20] = {0};
        sha256CalculateHash(hash, pathcopy, FS_MAX_PATH);
        std::string out = UL_BASE_SD_DIR "/nro/";
        std::stringstream strm;
        strm << out;
        for(u32 i = 0; i < 0x10; i++) {
            strm << std::setw(2) << std::setfill('0') << std::hex << std::nouppercase << (u32)hash[i];
        }
        strm << ".jpg";
        return strm.str();
    }
}