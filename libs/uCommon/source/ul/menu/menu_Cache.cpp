#include <ul/menu/menu_Cache.hpp>
#include <ul/ul_Result.hpp>

namespace ul::menu {

    namespace {

        // Identify homebrews by hashing their path and file size (the latter to re-cache basically any NROs that get modified)

        struct HomebrewCacheHashContext {
            char nro_path[FS_MAX_PATH];
            size_t file_size;
        };

        std::string GetHomebrewCachePath(const std::string &nro_path, const std::string &ext) {
            HomebrewCacheHashContext cache_hash_ctx = {};

            util::CopyToStringBuffer(cache_hash_ctx.nro_path, nro_path);
            cache_hash_ctx.file_size = fs::GetFileSize(nro_path);

            u8 hash[SHA256_HASH_SIZE] = {};
            sha256CalculateHash(hash, &cache_hash_ctx, sizeof(cache_hash_ctx));

            return fs::JoinPath(HomebrewCachePath, util::FormatSha256Hash(hash, true) + "." + ext);
        }

        void CacheHomebrewEntry(const std::string &nro_path) {
            const auto cache_nro_icon_path = GetHomebrewCacheIconPath(nro_path);
            const auto cache_nro_nacp_path = GetHomebrewCacheNacpPath(nro_path);
            if(fs::ExistsFile(cache_nro_icon_path) && fs::ExistsFile(cache_nro_nacp_path)) {
                // We know it's already cached
                return;
            }

            auto f = fopen(nro_path.c_str(), "rb");
            if(f) {
                if(fseek(f, sizeof(NroStart), SEEK_SET) == 0) {
                    NroHeader header = {};
                    if(fread(&header, sizeof(header), 1, f) == 1) {
                        if(fseek(f, header.size, SEEK_SET) == 0) {
                            NroAssetHeader asset_header = {};
                            if(fread(&asset_header, sizeof(asset_header), 1, f) == 1) {
                                if(asset_header.magic == NROASSETHEADER_MAGIC) {
                                    if(!fs::ExistsFile(cache_nro_icon_path) && (asset_header.icon.offset > 0) && (asset_header.icon.size > 0)) {
                                        auto icon_buf = new u8[asset_header.icon.size]();
                                        if(fseek(f, header.size + asset_header.icon.offset, SEEK_SET) == 0) {
                                            if(fread(icon_buf, asset_header.icon.size, 1, f) == 1) {
                                                fs::WriteFile(cache_nro_icon_path, icon_buf, asset_header.icon.size, true);
                                            }
                                        }
                                        delete[] icon_buf;
                                    }
                                    if(!fs::ExistsFile(cache_nro_nacp_path) && (asset_header.nacp.offset > 0) && (asset_header.nacp.size > 0)) {
                                        auto nacp_buf = new u8[asset_header.nacp.size]();
                                        if(fseek(f, header.size + asset_header.nacp.offset, SEEK_SET) == 0) {
                                            if(fread(nacp_buf, asset_header.nacp.size, 1, f) == 1) {
                                                fs::WriteFile(cache_nro_nacp_path, nacp_buf, asset_header.nacp.size, true);
                                            }
                                        }
                                        delete[] nacp_buf;
                                    }
                                }
                            }
                        }
                    }
                }
                fclose(f);
            }
        }

        void CacheHomebrewEntries(const std::string &hb_base_path) {
            UL_FS_FOR(hb_base_path, name, path, is_dir, is_file, {
                if(dt->d_type & DT_DIR) {
                    CacheHomebrewEntries(path);
                }
                else if(util::StringEndsWith(name, ".nro")) {
                    CacheHomebrewEntry(path);
                }
            });
        }

        bool CacheApplicationEntry(const u64 app_id, NsApplicationControlData *tmp_control_data) {
            const auto cache_icon_path = GetApplicationCacheIconPath(app_id);
            fs::DeleteFile(cache_icon_path);
            const auto rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, app_id, tmp_control_data, sizeof(NsApplicationControlData), nullptr);
            if(R_SUCCEEDED(rc)) {
                fs::WriteFile(cache_icon_path, tmp_control_data->icon, sizeof(tmp_control_data->icon), true);
                return true;
            }
            else {
                UL_LOG_WARN("Application cache failed: %s", util::FormatResultDisplay(rc).c_str());
                return false;
            }
        }

        void CacheApplicationEntries(const std::vector<NsApplicationRecord> &records) {
            auto tmp_control_data = new NsApplicationControlData();
            for(const auto &record: records) {
                CacheApplicationEntry(record.application_id, tmp_control_data);
            }
            delete tmp_control_data;
        }

    }

    void CacheHomebrew(const std::string &hb_base_path) {
        fs::CleanDirectory(HomebrewCachePath);
        CacheHomebrewEntry(HbmenuPath);
        CacheHomebrewEntries(hb_base_path);
    }

    void CacheApplications(const std::vector<NsApplicationRecord> &records) {
        fs::CleanDirectory(ApplicationCachePath);
        CacheApplicationEntries(records);
    }

    bool CacheSingleApplication(const u64 app_id) {
        auto tmp_control_data = new NsApplicationControlData();
        const auto ok = CacheApplicationEntry(app_id, tmp_control_data);
        delete tmp_control_data;
        return ok;
    }

    std::string GetHomebrewCacheIconPath(const std::string &nro_path) {
        return GetHomebrewCachePath(nro_path, "jpg");
    }

    std::string GetHomebrewCacheNacpPath(const std::string &nro_path) {
        return GetHomebrewCachePath(nro_path, "nacp");
    }
    
}
