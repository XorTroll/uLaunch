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
            if(fread(&hdr, 1, sizeof(NroHeader), f) == sizeof(NroHeader))
            {
                fseek(f, hdr.size, SEEK_SET);
                NroAssetHeader ahdr = {};
                if(fread(&ahdr, 1, sizeof(NroAssetHeader), f) == sizeof(NroAssetHeader))
                {
                    if(ahdr.magic == NROASSETHEADER_MAGIC)
                    {
                        if(ahdr.icon.size > 0)
                        {
                            u8 *iconbuf = new u8[ahdr.icon.size]();
                            fseek(f, hdr.size + ahdr.icon.offset, SEEK_SET);
                            if(fread(iconbuf, 1, ahdr.icon.size, f) == ahdr.icon.size) fs::WriteFile(nroimg, iconbuf, ahdr.icon.size, true);
                            delete[] iconbuf;
                        }
                    }
                }
            }
            fclose(f);
        }
    }

    std::vector<TitleRecord> QueryAllHomebrew(bool cache, std::string base)
    {
        std::vector<TitleRecord> nros;

        fs::ForEachFileIn(base, [&](std::string name, std::string path)
        {
            if(util::StringEndsWith(name, ".nro"))
            {
                auto sz = fs::GetFileSize(path);
                if(sz > 0)
                {
                    if(cache) CacheHomebrew(path);

                    TitleRecord rec = {};
                    rec.title_type = (u32)TitleType::Homebrew;
                    strcpy(rec.nro_target.nro_path, path.c_str());
                    nros.push_back(rec);
                }
            }
        });

        fs::ForEachDirectoryIn(base, [&](std::string name, std::string path)
        {
            auto hb = QueryAllHomebrew(cache, path);
            if(!hb.empty()) nros.insert(nros.begin(), hb.begin(), hb.end());
        });

        return nros;
    }

    std::string GetRecordIconPath(TitleRecord record)
    {
        std::string icon;
        if((TitleType)record.title_type == TitleType::Homebrew)
        {
            if(!record.icon.empty()) icon = record.icon;
            else icon = GetNROCacheIconPath(record.nro_target.nro_path);
        }
        else
        {
            if(!record.icon.empty()) icon = record.icon;
            else icon = GetTitleCacheIconPath(record.app_id);
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
        auto themedir = std::string(Q_THEMES_PATH) + "/" + base_name;
        if(base_name.empty()) themedir = CFG_THEME_DEFAULT;
        theme.path = themedir;
        auto metajson = themedir + "/theme/Manifest.json";
        auto [rc, meta] = util::LoadJSONFromFile(metajson);
        if(R_SUCCEEDED(rc))
        {
            theme.manifest.name = meta.value("name", "'" + base_name + "'");
            theme.manifest.format_version = meta.value("format_version", CurrentThemeFormatVersion);
            theme.manifest.release = meta.value("release", "");
            theme.manifest.description = meta.value("description", "");
            theme.manifest.author = meta.value("author", "");
        }
        return theme;
    }

    std::vector<Theme> LoadThemes()
    {
        std::vector<Theme> themes;

        fs::ForEachDirectoryIn(Q_THEMES_PATH, [&](std::string name, std::string path)
        {
            Theme t = LoadTheme(name);
            if(!t.path.empty()) themes.push_back(t);
        });

        return themes;
    }

    std::string ThemeResource(Theme &base, std::string resource_base)
    {
        auto base_res = base.path + "/" + resource_base;
        if(fs::ExistsFile(base_res)) return base_res;
        base_res = std::string(CFG_THEME_DEFAULT) + "/" + resource_base;
        if(fs::ExistsFile(base_res)) return base_res;
        return "";
    }

    std::string ProcessedThemeResource(ProcessedTheme &base, std::string resource_base)
    {
        return ThemeResource(base.base, resource_base);
    }

    ProcessedTheme ProcessTheme(Theme &base)
    {
        ProcessedTheme processed;
        processed.base = base;
        auto uijson = ThemeResource(base, "ui/UI.json");
        auto [rc, ui] = util::LoadJSONFromFile(uijson);
        if(R_SUCCEEDED(rc))
        {
            processed.ui.suspended_final_alpha = ui.value("suspended_final_alpha", 80);
            auto bgmjson = ThemeResource(base, "sound/BGM.json");
            auto [rc, bgm] = util::LoadJSONFromFile(bgmjson);
            if(R_SUCCEEDED(rc))
            {
                processed.sound.loop = bgm.value("loop", true);
                processed.sound.fade_in = bgm.value("fade_in", true);
                processed.sound.fade_out = bgm.value("fade_in", true);
            }
        }
        return processed;
    }

    Config CreateNewAndLoadConfig()
    {
        Config cfg = {};
        cfg.system_title_override_enabled = false; // Due to ban risk, have it disabled by default.
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
        std::ofstream ofs(CFG_CONFIG_JSON);
        ofs << j;
        ofs.close();
    }

    void SaveRecord(TitleRecord &record)
    {
        JSON entry = JSON::object();
        entry["type"] = record.title_type;
        entry["folder"] = record.sub_folder;

        // Prepare JSON path
        std::string basepath = Q_ENTRIES_PATH;
        std::string json = basepath;
        if((TitleType)record.title_type == TitleType::Homebrew)
        {
            auto jsonname = std::to_string(fs::GetFileSize(record.nro_target.nro_path)) + ".json";
            if(record.json_name.empty()) record.json_name = jsonname;
            json += "/" + jsonname;
            entry["nro_path"] = record.nro_target.nro_path;
            if(strcmp(record.nro_target.nro_path, record.nro_target.argv) != 0) entry["nro_argv"] = record.nro_target.argv;
            if(!record.icon.empty()) entry["icon"] = record.icon;
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
        ofs << entry;
        ofs.close();
    }

    bool MoveTitleToDirectory(TitleList &list, u64 app_id, std::string folder)
    {
        bool title_found = false;
        TitleRecord record_copy = {};
        std::string recjson;

        // Search in root first
        auto find = STLITER_FINDWITHCONDITION(list.root.titles, tit, (tit.app_id == app_id));
        if(STLITER_ISFOUND(list.root.titles, find))
        {
            if(folder.empty()) return true;  // It is already on root...?
            recjson = STLITER_UNWRAP(find).json_name;

            list.root.titles.erase(find);
            title_found = true;
        }

        if(!title_found) // If not found yet, search on all dirs if the title is present
        {
            for(auto &fld: list.folders)
            {
                auto find = STLITER_FINDWITHCONDITION(fld.titles, entry, (entry.app_id == app_id));
                if(STLITER_ISFOUND(fld.titles, find))
                {
                    if(fld.name == folder) return true; // It is already on that folder...?
                    recjson = STLITER_UNWRAP(find).json_name;

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
                auto find2 = STLITER_FINDWITHCONDITION(list.folders, fld, (fld.name == folder));
                if(STLITER_ISFOUND(list.folders, find2))
                {
                    STLITER_UNWRAP(find2).titles.push_back(title);
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
            auto find = STLITER_FINDWITHCONDITION(list.root.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
            if(STLITER_ISFOUND(list.root.titles, find))
            {
                if(folder.empty()) return true;  // It is already on root...?
                recjson = STLITER_UNWRAP(find).json_name;

                list.root.titles.erase(find);
                title_found = true;
            }
        }
        else
        {
            auto find = STLITER_FINDWITHCONDITION(list.root.titles, tit, (tit.title_type == record.title_type) && (strcmp(tit.nro_target.nro_path, record.nro_target.nro_path) == 0));
            if(STLITER_ISFOUND(list.root.titles, find))
            {
                if(folder.empty()) return true;  // It is already on root...?
                recjson = STLITER_UNWRAP(find).json_name;

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
                    auto find = STLITER_FINDWITHCONDITION(fld.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
                    if(STLITER_ISFOUND(fld.titles, find))
                    {
                        if(fld.name == folder) return true; // It is already on that folder...?
                        recjson = STLITER_UNWRAP(find).json_name;

                        fld.titles.erase(find);
                        title_found = true;
                        break;
                    }
                }
                else
                {
                    auto find = STLITER_FINDWITHCONDITION(fld.titles, tit, (tit.title_type == record.title_type) && (strcmp(tit.nro_target.nro_path, record.nro_target.nro_path) == 0));
                    if(STLITER_ISFOUND(fld.titles, find))
                    {
                        if(fld.name == folder) return true; // It is already on that folder...?
                        recjson = STLITER_UNWRAP(find).json_name;

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
                auto find2 = STLITER_FINDWITHCONDITION(list.folders, fld, (fld.name == folder));
                if(STLITER_ISFOUND(list.folders, find2))
                {
                    STLITER_UNWRAP(find2).titles.push_back(title);
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
            auto f = STLITER_FINDWITHCONDITION(list.folders, fld, (fld.name == name));
            if(STLITER_ISFOUND(list.folders, f))
            {
                return STLITER_UNWRAP(f);
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
            auto find = STLITER_FINDWITHCONDITION(list.root.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
            if(STLITER_ISFOUND(list.root.titles, find))
            {
                if(!STLITER_UNWRAP(find).json_name.empty()) title_found = true;
            }
        }
        else
        {
            auto find = STLITER_FINDWITHCONDITION(list.root.titles, tit, (tit.title_type == record.title_type) && (strcmp(tit.nro_target.nro_path, record.nro_target.nro_path) == 0));
            if(STLITER_ISFOUND(list.root.titles, find))
            {
                if(!STLITER_UNWRAP(find).json_name.empty()) title_found = true;
            }
        }

        if(!title_found) // If not found yet, search on all dirs if the title is present
        {
            for(auto &fld: list.folders)
            {
                if((TitleType)record.title_type == TitleType::Installed)
                {
                    auto find = STLITER_FINDWITHCONDITION(fld.titles, tit, (tit.title_type == record.title_type) && (tit.app_id == record.app_id));
                    if(STLITER_ISFOUND(fld.titles, find))
                    {
                        if(!STLITER_UNWRAP(find).json_name.empty())
                        {
                            title_found = true;
                            break;
                        }
                    }
                }
                else
                {
                    auto find = STLITER_FINDWITHCONDITION(fld.titles, tit, (tit.title_type == record.title_type) && (strcmp(tit.nro_target.nro_path, record.nro_target.nro_path) == 0));
                    if(STLITER_ISFOUND(fld.titles, find))
                    {
                        if(!STLITER_UNWRAP(find).json_name.empty())
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

    ResultWith<TitleList> LoadTitleList(bool cache)
    {
        TitleList list = {};
        
        // Installed titles first
        auto [rc, titles] = os::QueryInstalledTitles(cache);
        if(R_SUCCEEDED(rc))
        {
            for(auto &title: titles)
            {
                list.root.titles.push_back(title);
            }
        }
        else return MakeResultWith(rc, list);

        fs::ForEachFileIn(Q_ENTRIES_PATH, [&](std::string name, std::string path)
        {
            auto [rc, entry] = util::LoadJSONFromFile(path);
            if(R_SUCCEEDED(rc))
            {
                TitleType type = (TitleType)entry.value("type", 0u);
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

                                auto find = STLITER_FINDWITHCONDITION(list.root.titles, tit, (tit.app_id == appid));
                                if(STLITER_ISFOUND(list.root.titles, find))
                                {
                                    list.root.titles.erase(find);
                                }

                                auto find2 = STLITER_FINDWITHCONDITION(list.folders, fld, (fld.name == folder));
                                if(STLITER_ISFOUND(list.folders, find2))
                                {
                                    STLITER_UNWRAP(find2).titles.push_back(rec);
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
                    if(!nropath.empty())
                    {
                        TitleRecord rec = {};
                        rec.json_name = name;
                        rec.title_type = (u32)type;
                        std::string argv = entry.value("nro_argv", nropath);
                        strcpy(rec.nro_target.nro_path, nropath.c_str());
                        strcpy(rec.nro_target.argv, argv.c_str());
                        std::string folder = entry.value("folder", "");
                        rec.sub_folder = folder;
                        rec.icon = entry.value("icon", "");
                        if(cache) CacheHomebrew(rec.nro_target.nro_path);
                        if(folder.empty()) list.root.titles.push_back(rec);
                        else
                        {
                            auto find = STLITER_FINDWITHCONDITION(list.folders, fld, (fld.name == folder));
                            if(STLITER_ISFOUND(list.folders, find)) STLITER_UNWRAP(find).titles.push_back(rec);
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
        });

        return SuccessResultWith(list);
    }

    std::string GetTitleCacheIconPath(u64 app_id)
    {
        auto strappid = util::FormatApplicationId(app_id);
        return Q_BASE_SD_DIR "/title/" + strappid + ".jpg";
    }

    std::string GetNROCacheIconPath(std::string path)
    {
        auto fsz = fs::GetFileSize(path);
        return Q_BASE_SD_DIR "/nro/" + std::to_string(fsz) + ".jpg";
    }
}