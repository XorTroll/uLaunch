#include <cfg/cfg_Config.hpp>
#include <os/os_Titles.hpp>
#include <util/util_JSON.hpp>
#include <util/util_String.hpp>
#include <fs/fs_Stdio.hpp>
#include <util/util_Convert.hpp>

namespace cfg
{
    bool MoveTitleToDirectory(TitleList &list, u64 app_id, std::string folder)
    {
        bool title_found = false;

        // Search in root first
        auto find = STLITER_FINDWITHCONDITION(list.root.titles, tit, (tit.app_id == app_id));
        if(STLITER_ISFOUND(list.root.titles, find))
        {
            if(folder.empty()) return true;  // It is already on root...?

            list.root.titles.erase(find);
            title_found = true;
        }

        if(!title_found) // If not found yet, search on all dirs if the title is present
        {
            for(auto &fld: list.folders)
            {
                auto find = STLITER_FINDWITHCONDITION(fld.titles, item, (item.app_id == app_id));
                if(STLITER_ISFOUND(fld.titles, find))
                {
                    if(fld.name == folder) return true; // It is already on that folder...?

                    fld.titles.erase(find);
                    title_found = true;
                }
            }
        }

        if(title_found) 
        {
            TitleRecord title = {};
            title.app_id = app_id;
            title.title_type = (u32)TitleType::Installed;

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

        auto [rc2, menu] = util::LoadJSONFromFile(Q_MENU_JSON);
        if(R_SUCCEEDED(rc2))
        {
            for(auto &item: menu)
            {
                TitleType type = (TitleType)item.value("type", 0u);
                if(type == TitleType::Installed)
                {
                    std::string appidstr = item.value("application_id", "");
                    if(!appidstr.empty())
                    {
                        std::string folder = item.value("folder", "");
                        u64 appid = util::Get64FromString(appidstr);
                        if(appid > 0)
                        {
                            if(!folder.empty())
                            {
                                TitleRecord rec = {};
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
                    std::string nropath = item.value("nro_path", "");
                    if(!nropath.empty())
                    {
                        TitleRecord rec = {};
                        rec.title_type = (u32)type;
                        rec.nro_path = "sdmc:";
                        if(nropath.front() != '/') rec.nro_path += "/";
                        rec.nro_path += nropath;
                        std::string folder = item.value("folder", "");
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
                                                        fs::DeleteFile(nroimg);
                                                        FILE *iconf = fopen(nroimg.c_str(), "wb");
                                                        if(iconf)
                                                        {
                                                            fwrite(iconbuf, 1, ahdr.icon.size, iconf);
                                                            fclose(iconf);
                                                        }
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
        }



        // Search SD card for installed title extensions or homebrew accessors
        fs::ForEachFileIn(Q_BASE_SD_DIR "/entries", [&](std::string name, std::string path)
        {
            if(util::StringEndsWith(name, ".json"))
            {
                auto [rc, json] = util::LoadJSONFromFile(path);
                if(R_SUCCEEDED(rc))
                {
                    TitleType type = (TitleType)json.value("type", 0u);
                    if(type == TitleType::Installed)
                    {
                        std::string appidstr = json.value("application_id", "");
                        if(!appidstr.empty())
                        {
                            std::string folder = json.value("folder", "");
                            u64 appid = util::Get64FromString(appidstr);
                            if(appid > 0)
                            {
                                if(!folder.empty()) MoveTitleToDirectory(list, appid, folder);
                            }
                        }
                    }
                    else if(type == TitleType::Homebrew)
                    {
                        std::string nropath = json.value("nro_path", "");
                        if(!nropath.empty())
                        {
                            TitleRecord rec = {};
                            rec.title_type = (u32)type;
                            rec.nro_path = "sdmc:";
                            if(nropath.front() != '/') rec.nro_path += "/";
                            rec.nro_path += nropath;
                            std::string folder = json.value("folder", "");
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
                                                            fs::DeleteFile(nroimg);
                                                            FILE *iconf = fopen(nroimg.c_str(), "wb");
                                                            if(iconf)
                                                            {
                                                                fwrite(iconbuf, 1, ahdr.icon.size, iconf);
                                                                fclose(iconf);
                                                            }
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