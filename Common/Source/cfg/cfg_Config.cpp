#include <cfg/cfg_Config.hpp>
#include <os/os_Titles.hpp>
#include <util/util_Misc.hpp>
#include <util/util_String.hpp>
#include <fs/fs_Stdio.hpp>
#include <db/db_Save.hpp>
#include <util/util_Convert.hpp>

namespace cfg
{
    void CacheHomebrew(std::string nro_path)
    {
        auto nroimg = GetNROCacheIconPath(nro_path);
        FILE *f = fopen(nro_path.c_str(), "rb");
        if(f)
        {
            fseek(f, sizeof(NroStart), SEEK_SET);
            NroHeader hdr = {};
            if(fread(&hdr, sizeof(NroHeader), 1, f) == 1)
            {
                fseek(f, hdr.size, SEEK_SET);
                NroAssetHeader ahdr = {};
                if(fread(&ahdr, sizeof(NroAssetHeader), 1, f) == 1)
                {
                    if(ahdr.magic == NROASSETHEADER_MAGIC)
                    {
                        if((ahdr.icon.offset > 0) && (ahdr.icon.size > 0))
                        {
                            u8 *iconbuf = new u8[ahdr.icon.size]();
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

    std::vector<TitleRecord> QueryAllHomebrew(std::string base)
    {
        std::vector<TitleRecord> nros;

        FS_FOR(base, name, path,
        {
            if(dt->d_type & DT_DIR)
            {
                auto hb = QueryAllHomebrew(path);
                if(!hb.empty()) nros.insert(nros.begin(), hb.begin(), hb.end());
            }
            else
            {
                if(util::StringEndsWith(name, ".nro"))
                {
                    TitleRecord rec = {};
                    rec.title_type = (u32)TitleType::Homebrew;
                    strcpy(rec.nro_target.nro_path, path.c_str());
                    nros.push_back(rec);
                    CacheHomebrew(path); // Always cache when querying.
                }
            }
        })

        return nros;
    }

    std::string GetRecordIconPath(TitleRecord record)
    {
        std::string icon = record.icon;
        if(icon.empty())
        {
            if((TitleType)record.title_type == TitleType::Homebrew) icon = GetNROCacheIconPath(record.nro_target.nro_path);
            else if((TitleType)record.title_type == TitleType::Installed) icon = GetTitleCacheIconPath(record.app_id);
        }
        
        return icon;
    }

    RecordInformation GetRecordInformation(TitleRecord record)
    {
        RecordInformation info = {};
        info.icon_path = GetRecordIconPath(record);
        if((TitleType)record.title_type == TitleType::Homebrew)
        {
            FILE *f = fopen(record.nro_target.nro_path, "rb");
            if(f)
            {
                fseek(f, sizeof(NroStart), SEEK_SET);
                NroHeader hdr = {};
                if(fread(&hdr, 1, sizeof(NroHeader), f) == sizeof(NroHeader))
                {
                    fseek(f, hdr.size, SEEK_SET);
                    NroAssetHeader ahdr = {};
                    if(fread(&ahdr, 1, sizeof(NroAssetHeader), f) == sizeof(NroAssetHeader))
                    {
                        if(ahdr.magic == NROASSETHEADER_MAGIC)
                        {
                            if(ahdr.nacp.size > 0)
                            {
                                fseek(f, hdr.size + ahdr.nacp.offset, SEEK_SET);
                                fread(&info.nacp, 1, ahdr.nacp.size, f);
                            }
                        }
                    }
                }
                fclose(f);
            }
        }
        else
        {
            NsApplicationControlData cdata = {};
            nsGetApplicationControlData(1, record.app_id, &cdata, sizeof(cdata), NULL);
            memcpy(&info.nacp, &cdata.nacp, sizeof(cdata.nacp));
        }
        if(!record.name.empty())
        {
            for(u32 i = 0; i < 0x10; i++) strcpy(info.nacp.lang[i].name, record.name.c_str());
        }
        if(!record.author.empty())
        {
            for(u32 i = 0; i < 0x10; i++) strcpy(info.nacp.lang[i].author, record.author.c_str());
        }
        if(!record.version.empty()) strcpy(info.nacp.version, record.version.c_str());
        return info;
    }

    NacpLanguageEntry *GetRecordInformationLanguageEntry(RecordInformation &info)
    {
        NacpLanguageEntry *lent = NULL;
        nacpGetLanguageEntry(&info.nacp, &lent);
        if(lent == NULL)
        {
            for(u32 i = 0; i < 16; i++)
            {
                lent = &info.nacp.lang[i];
                if(strlen(lent->name) && strlen(lent->author)) break;
                lent = NULL;
            }
        }
        return lent;
    }

    Theme LoadTheme(std::string base_name)
    {
        Theme theme = {};
        theme.base_name = base_name;
        auto themedir = std::string(Q_THEMES_PATH) + "/" + base_name;
        auto metajson = themedir + "/theme/Manifest.json";
        if(base_name.empty() || !fs::ExistsFile(metajson)) themedir = CFG_THEME_DEFAULT;
        metajson = themedir + "/theme/Manifest.json";
        auto [rc, meta] = util::LoadJSONFromFile(metajson);
        if(R_SUCCEEDED(rc))
        {
            theme.manifest.name = meta.value("name", "'" + base_name + "'");
            theme.manifest.format_version = meta.value("format_version", 0);
            theme.manifest.release = meta.value("release", "");
            theme.manifest.description = meta.value("description", "");
            theme.manifest.author = meta.value("author", "");
            theme.path = themedir;
        }
        else return LoadTheme("");
        return theme;
    }

    std::vector<Theme> LoadThemes()
    {
        std::vector<Theme> themes;

        FS_FOR(std::string(Q_THEMES_PATH), name, path,
        {
            Theme t = LoadTheme(name);
            if(!t.path.empty()) themes.push_back(t);
        })

        return themes;
    }

    std::string GetAssetByTheme(Theme &base, std::string resource_base)
    {
        auto base_res = base.path + "/" + resource_base;
        if(fs::ExistsFile(base_res)) return base_res;
        base_res = std::string(CFG_THEME_DEFAULT) + "/" + resource_base;
        if(fs::ExistsFile(base_res)) return base_res;
        return "";
    }

    std::string GetLanguageJSONPath(std::string lang)
    {
        return Q_BASE_SD_DIR "/lang/" + lang + ".json";
    }

    std::string GetLanguageString(JSON &lang, JSON &def, std::string name)
    {
        auto str = lang.value(name, "");
        if(str.empty()) str = def.value(name, "");
        return str;
    }

    Config CreateNewAndLoadConfig()
    {
        Config cfg = {};
        cfg.system_title_override_enabled = false; // Due to ban risk, have it disabled by default.
        cfg.viewer_usb_enabled = false; // Do not enable this by default due to conflicts with USB homebrew
        cfg.theme_name = ""; // Default theme (none)
        SaveConfig(cfg);
        return cfg;
    }

    Config LoadConfig()
    {
        Config cfg = {};
        auto [rc, cfgjson] = util::LoadJSONFromFile(CFG_CONFIG_JSON);
        if(R_SUCCEEDED(rc))
        {
            cfg.theme_name = cfgjson.value("theme_name", "");
            cfg.system_title_override_enabled = cfgjson.value("system_title_override_enabled", false);
            cfg.viewer_usb_enabled = cfgjson.value("viewer_usb_enabled", false);
        }
        else
        {
            fs::DeleteFile(CFG_CONFIG_JSON);
            return CreateNewAndLoadConfig();
        }
        return cfg;
    }

    Config EnsureConfig()
    {
        if(!fs::ExistsFile(CFG_CONFIG_JSON)) return CreateNewAndLoadConfig();
        return LoadConfig();
    }

    void SaveConfig(Config &cfg)
    {
        fs::DeleteFile(CFG_CONFIG_JSON);
        JSON j = JSON::object();
        j["theme_name"] = cfg.theme_name;
        j["system_title_override_enabled"] = cfg.system_title_override_enabled;
        j["viewer_usb_enabled"] = cfg.viewer_usb_enabled;
        std::ofstream ofs(CFG_CONFIG_JSON);
        ofs << std::setw(4) << j;
        ofs.close();
    }

    void SaveRecord(TitleRecord &record)
    {
        JSON entry = JSON::object();
        entry["type"] = record.title_type;
        entry["folder"] = record.sub_folder;

        if(!record.name.empty()) entry["name"] = record.name;
        if(!record.author.empty()) entry["author"] = record.author;
        if(!record.version.empty()) entry["version"] = record.version;
        if(!record.icon.empty()) entry["icon"] = record.icon;

        // Prepare JSON path
        std::string basepath = Q_ENTRIES_PATH;
        std::string json = basepath;
        if((TitleType)record.title_type == TitleType::Homebrew)
        {
            auto jsonname = std::to_string(fs::GetFileSize(record.nro_target.nro_path)) + ".json";
            if(record.json_name.empty()) record.json_name = jsonname;
            json += "/" + jsonname;
            entry["nro_path"] = record.nro_target.nro_path;
            if(strlen(record.nro_target.argv))
            {
                if(strcmp(record.nro_target.nro_path, record.nro_target.argv) != 0) entry["nro_argv"] = record.nro_target.argv;
            }
        }
        else if((TitleType)record.title_type == TitleType::Installed)
        {
            auto strappid = util::FormatApplicationId(record.app_id);
            auto jsonname = strappid + ".json";
            if(record.json_name.empty()) record.json_name = jsonname;
            json += "/" + jsonname;
            entry["application_id"] = strappid;
        }
        if(!record.json_name.empty()) json = basepath + "/" + record.json_name;
        if(fs::ExistsFile(json)) fs::DeleteFile(json);

        std::ofstream ofs(json);
        ofs << std::setw(4) << entry;
        ofs.close();
    }

    void RemoveRecord(TitleRecord &record)
    {
        // Prepare JSON path
        std::string basepath = Q_ENTRIES_PATH;
        std::string json = basepath;
        if((TitleType)record.title_type == TitleType::Homebrew)
        {
            auto jsonname = std::to_string(fs::GetFileSize(record.nro_target.nro_path)) + ".json";
            if(record.json_name.empty()) record.json_name = jsonname;
            json += "/" + jsonname;
        }
        else if((TitleType)record.title_type == TitleType::Installed)
        {
            auto strappid = util::FormatApplicationId(record.app_id);
            auto jsonname = strappid + ".json";
            if(record.json_name.empty()) record.json_name = jsonname;
            json += "/" + jsonname;
        }
        if(!record.json_name.empty()) json = basepath + "/" + record.json_name;
        fs::DeleteFile(json);
    }

    bool MoveTitleToDirectory(TitleList &list, u64 app_id, std::string folder)
    {
        bool title_found = false;
        TitleRecord record_copy = {};
        std::string recjson;

        // Search in root first
        auto find = STL_FIND_IF(list.root.titles, tit, (tit.app_id == app_id));
        if(STL_FOUND(list.root.titles, find))
        {
            if(folder.empty()) return true;  // It is already on root...?
            recjson = STL_UNWRAP(find).json_name;

            list.root.titles.erase(find);
            title_found = true;
        }

        if(!title_found) // If not found yet, search on all dirs if the title is present
        {
            for(auto &fld: list.folders)
            {
                auto find = STL_FIND_IF(fld.titles, entry, (entry.app_id == app_id));
                if(STL_FOUND(fld.titles, find))
                {
                    if(fld.name == folder) return true; // It is already on that folder...?
                    recjson = STL_UNWRAP(find).json_name;

                    fld.titles.erase(find);
                    title_found = true;
                    break;
                }
            }
        }

        if(title_found) 
        {
            TitleRecord title = {};
            title.app_id = app_id;
            title.title_type = (u32)TitleType::Installed;
            title.json_name = recjson;
            title.sub_folder = folder;

            if(folder.empty()) // Add (move) it to root again
            {
                list.root.titles.push_back(title);
            }
            else // Add it to the new folder
            {
                auto find2 = STL_FIND_IF(list.folders, fld, (fld.name == folder));
                if(STL_FOUND(list.folders, find2))
                {
                    STL_UNWRAP(find2).titles.push_back(title);
                }
                else
                {
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

    bool MoveRecordTo(TitleList &list, TitleRecord record, std::string folder)
    {
        bool title_found = false;
        TitleRecord record_copy = {};
        std::string recjson;

        // Search in root first
        if((TitleType)record.title_type == TitleType::Installed)
        {
            auto find = STL_FIND_IF(list.root.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
            if(STL_FOUND(list.root.titles, find))
            {
                if(folder.empty()) return true;  // It is already on root...?
                recjson = STL_UNWRAP(find).json_name;

                list.root.titles.erase(find);
                title_found = true;
            }
        }
        else
        {
            auto find = STL_FIND_IF(list.root.titles, tit, (tit.title_type == record.title_type) && (strcmp(tit.nro_target.nro_path, record.nro_target.nro_path) == 0));
            if(STL_FOUND(list.root.titles, find))
            {
                if(folder.empty()) return true;  // It is already on root...?
                recjson = STL_UNWRAP(find).json_name;

                list.root.titles.erase(find);
                title_found = true;
            }
        }

        if(!title_found) // If not found yet, search on all dirs if the title is present
        {
            for(auto &fld: list.folders)
            {
                if((TitleType)record.title_type == TitleType::Installed)
                {
                    auto find = STL_FIND_IF(fld.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
                    if(STL_FOUND(fld.titles, find))
                    {
                        if(fld.name == folder) return true; // It is already on that folder...?
                        recjson = STL_UNWRAP(find).json_name;

                        fld.titles.erase(find);
                        title_found = true;
                        break;
                    }
                }
                else
                {
                    auto find = STL_FIND_IF(fld.titles, tit, (tit.title_type == record.title_type) && (strcmp(tit.nro_target.nro_path, record.nro_target.nro_path) == 0));
                    if(STL_FOUND(fld.titles, find))
                    {
                        if(fld.name == folder) return true; // It is already on that folder...?
                        recjson = STL_UNWRAP(find).json_name;

                        fld.titles.erase(find);
                        title_found = true;
                        break;
                    }
                }
            }
        }

        if(title_found) 
        {
            TitleRecord title = record;
            title.json_name = recjson;
            title.sub_folder = folder;

            if(folder.empty()) // Add (move) it to root again
            {
                list.root.titles.push_back(title);
            }
            else // Add it to the new folder
            {
                auto find2 = STL_FIND_IF(list.folders, fld, (fld.name == folder));
                if(STL_FOUND(list.folders, find2))
                {
                    STL_UNWRAP(find2).titles.push_back(title);
                }
                else
                {
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

    TitleFolder &FindFolderByName(TitleList &list, std::string name)
    {
        if(!name.empty())
        {
            auto f = STL_FIND_IF(list.folders, fld, (fld.name == name));
            if(STL_FOUND(list.folders, f))
            {
                return STL_UNWRAP(f);
            }
        }
        return list.root;
    }

    bool ExistsRecord(TitleList &list, TitleRecord record)
    {
        bool title_found = false;
        TitleRecord record_copy = {};
        std::string recjson;

        // Search in root first
        if((TitleType)record.title_type == TitleType::Installed)
        {
            auto find = STL_FIND_IF(list.root.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
            if(STL_FOUND(list.root.titles, find))
            {
                if(!STL_UNWRAP(find).json_name.empty()) title_found = true;
            }
        }
        else
        {
            auto find = STL_FIND_IF(list.root.titles, tit, (tit.title_type == record.title_type) && (strcmp(tit.nro_target.nro_path, record.nro_target.nro_path) == 0));
            if(STL_FOUND(list.root.titles, find))
            {
                if(!STL_UNWRAP(find).json_name.empty()) title_found = true;
            }
        }

        if(!title_found) // If not found yet, search on all dirs if the title is present
        {
            for(auto &fld: list.folders)
            {
                if((TitleType)record.title_type == TitleType::Installed)
                {
                    auto find = STL_FIND_IF(fld.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
                    if(STL_FOUND(fld.titles, find))
                    {
                        if(!STL_UNWRAP(find).json_name.empty())
                        {
                            title_found = true;
                            break;
                        }
                    }
                }
                else
                {
                    auto find = STL_FIND_IF(fld.titles, tit, (tit.title_type == record.title_type) && (strcmp(tit.nro_target.nro_path, record.nro_target.nro_path) == 0));
                    if(STL_FOUND(fld.titles, find))
                    {
                        if(!STL_UNWRAP(find).json_name.empty())
                        {
                            title_found = true;
                            break;
                        }
                    }
                }
            }
        }

        return title_found;
    }

    TitleList LoadTitleList(bool cache)
    {
        TitleList list = {};
        
        // Installed titles first
        auto titles = os::QueryInstalledTitles(cache);

        FS_FOR(std::string(Q_ENTRIES_PATH), name, path,
        {
            auto [rc, entry] = util::LoadJSONFromFile(path);
            if(R_SUCCEEDED(rc))
            {
                TitleType type = (TitleType)entry.value("type", 0);
                if(type == TitleType::Installed)
                {
                    std::string appidstr = entry.value("application_id", "");
                    if(!appidstr.empty())
                    {
                        std::string folder = entry.value("folder", "");
                        u64 appid = util::Get64FromString(appidstr);
                        if(appid > 0)
                        {
                            if(!folder.empty())
                            {
                                TitleRecord rec = {};
                                rec.json_name = name;
                                rec.app_id = appid;
                                rec.title_type = (u32)TitleType::Installed;
                                rec.name = entry.value("name", "");
                                rec.author = entry.value("author", "");
                                rec.version = entry.value("version", "");
                                rec.icon = entry.value("icon", "");

                                auto find = STL_FIND_IF(titles, tit, (tit.app_id == appid));
                                if(STL_FOUND(titles, find))
                                {
                                    titles.erase(find);
                                }

                                auto find2 = STL_FIND_IF(list.folders, fld, (fld.name == folder));
                                if(STL_FOUND(list.folders, find2))
                                {
                                    STL_UNWRAP(find2).titles.push_back(rec);
                                }
                                else
                                {
                                    TitleFolder fld = {};
                                    fld.name = folder;
                                    fld.titles.push_back(rec);
                                    list.folders.push_back(fld);
                                }
                            }
                        }
                    }
                }
                else if(type == TitleType::Homebrew)
                {
                    std::string nropath = entry.value("nro_path", "");
                    if(!nropath.empty() && fs::ExistsFile(nropath))
                    {
                        TitleRecord rec = {};
                        rec.json_name = name;
                        rec.title_type = (u32)type;
                        rec.name = entry.value("name", "");
                        rec.author = entry.value("author", "");
                        rec.version = entry.value("version", "");

                        // Only cache homebrew added to main menu.
                        CacheHomebrew(nropath);
                        std::string argv = entry.value("nro_argv", "");
                        strcpy(rec.nro_target.nro_path, nropath.c_str());
                        if(!argv.empty()) strcpy(rec.nro_target.argv, argv.c_str());
                        std::string folder = entry.value("folder", "");
                        rec.sub_folder = folder;
                        rec.icon = entry.value("icon", "");
                        
                        if(folder.empty()) list.root.titles.push_back(rec);
                        else
                        {
                            auto find = STL_FIND_IF(list.folders, fld, (fld.name == folder));
                            if(STL_FOUND(list.folders, find)) STL_UNWRAP(find).titles.push_back(rec);
                            else
                            {
                                TitleFolder fld = {};
                                fld.name = folder;
                                fld.titles.push_back(rec);
                                list.folders.push_back(fld);
                            }
                        }
                    }
                }
            }
        })

        for(auto &title: titles)
        {
            list.root.titles.push_back(title);
        }

        return list;
    }

    std::string GetTitleCacheIconPath(u64 app_id)
    {
        auto strappid = util::FormatApplicationId(app_id);
        return Q_BASE_SD_DIR "/title/" + strappid + ".jpg";
    }

    std::string GetNROCacheIconPath(std::string path)
    {
        char pathcopy[FS_MAX_PATH] = {0};
        strcpy(pathcopy, path.c_str());
        char hash[0x20] = {0};
        sha256CalculateHash(hash, pathcopy, FS_MAX_PATH);
        std::string out = Q_BASE_SD_DIR "/nro/";
        std::stringstream strm;
        strm << out;
        for(u32 i = 0; i < 0x10; i++) strm << std::setw(2) << std::setfill('0') << std::hex << std::nouppercase << (int)hash[i];
        strm << ".jpg";
        return strm.str();
    }
}