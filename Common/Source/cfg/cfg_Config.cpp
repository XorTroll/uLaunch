#include <cfg/cfg_Config.hpp>
#include <os/os_Titles.hpp>
#include <util/util_JSON.hpp>
#include <util/util_String.hpp>
#include <fs/fs_Stdio.hpp>
#include <db/db_Save.hpp>
#include <util/util_Convert.hpp>

namespace cfg
{
    void SaveRecord(TitleRecord record)
    {
        JSON entry = JSON::object();
        entry["type"] = record.title_type;
        entry["folder"] = record.sub_folder;

        // Prepare JSON path
        std::string basepath = Q_ENTRIES_PATH;
        std::string json = basepath;
        if((TitleType)record.title_type == TitleType::Homebrew)
        {
            json += "/" + std::to_string(fs::GetFileSize(record.nro_path)) + ".json";
            entry["nro_path"] = record.nro_path;
        }
        else if((TitleType)record.title_type == TitleType::Installed)
        {
            auto strappid = util::FormatApplicationId(record.app_id);
            json += "/" + strappid + ".json";
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
                        rec.nro_path = "sdmc:";
                        if(nropath.front() != '/') rec.nro_path += "/";
                        rec.nro_path += nropath;
                        std::string folder = entry.value("folder", "");
                        rec.sub_folder = folder;
                        if(cache)
                        {
                            auto nroimg = GetNROCacheIconPath(rec.nro_path);
                            FILE *f = fopen(rec.nro_path.c_str(), "rb");
                            if(f)
                            {
                                NroStart st = {};
                                if(fread(&st, sizeof(NroStart), 1, f) == 1)
                                {
                                    NroHeader hdr = {};
                                    if(fread(&hdr, sizeof(NroHeader), 1, f) == 1)
                                    {
                                        fseek(f, hdr.size, SEEK_SET);
                                        NroAssetHeader ahdr = {};
                                        if(fread(&ahdr, sizeof(NroAssetHeader), 1, f) == 1)
                                        {
                                            if(ahdr.magic == NROASSETHEADER_MAGIC)
                                            {
                                                if(ahdr.icon.size > 0)
                                                {
                                                    u8 *iconbuf = new u8[ahdr.icon.size]();
                                                    fseek(f, hdr.size + ahdr.icon.size, SEEK_SET);
                                                    if(fread(iconbuf, ahdr.icon.size, 1, f) == 1)
                                                    {
                                                        fs::WriteFile(nroimg, iconbuf, ahdr.icon.size, true);
                                                    }
                                                    delete[] iconbuf;
                                                }
                                            }
                                        }
                                    }
                                }
                                fclose(f);
                            }
                        }
                        if(folder.empty())
                        {
                            list.root.titles.push_back(rec);
                        }
                        else
                        {
                            auto find = STLITER_FINDWITHCONDITION(list.folders, fld, (fld.name == folder));
                            if(STLITER_ISFOUND(list.folders, find))
                            {
                                STLITER_UNWRAP(find).titles.push_back(rec);
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