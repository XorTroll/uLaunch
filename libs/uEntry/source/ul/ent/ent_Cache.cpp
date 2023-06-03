#include <ul/ent/ent_Cache.hpp>
#include <ul/ent/ent_Entry.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_String.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Scope.hpp>
#include <ul/util/util_Vector.hpp>
#include <extras/json.hpp>

namespace ul::ent {

    namespace {

        bool IsValidHomebrew(const std::string &nro_path, NroHeader &header) {
            if(!fs::ReadFileAtOffset(nro_path, sizeof(NroStart), std::addressof(header), sizeof(header))) {
                return false;
            }
            if(header.magic != NROHEADER_MAGIC) {
                return false;
            }

            return true;
        }

        bool ReadHomebrewAssetInfo(const std::string &nro_path, u8 *&out_icon_buf, size_t &out_icon_size, NacpStruct &out_nacp) {
            NroHeader header;
            if(!IsValidHomebrew(nro_path, header)) {
                return false;
            }

            auto load_default_icon = true;
            auto load_default_nacp = true;
            NroAssetHeader asset_header;
            if(fs::ReadFileAtOffset(nro_path, header.size, &asset_header, sizeof(asset_header))) {
                if(asset_header.magic == NROASSETHEADER_MAGIC) {
                    if((asset_header.nacp.offset > 0) && (asset_header.nacp.size == sizeof(out_nacp))) {
                        if(fs::ReadFileAtOffset(nro_path, header.size + asset_header.nacp.offset, std::addressof(out_nacp), sizeof(out_nacp))) {
                            load_default_nacp = false;
                        }
                    }

                    if((asset_header.icon.offset > 0) && (asset_header.icon.size > 0)) {
                        out_icon_size = asset_header.icon.size;
                        out_icon_buf = new u8[out_icon_size]();
                        if(fs::ReadFileAtOffset(nro_path, header.size + asset_header.icon.offset, out_icon_buf, out_icon_size)) {
                            load_default_icon = false;
                        }
                        else {
                            delete[] out_icon_buf;
                        }
                    }
                }
            }

            if(load_default_icon) {
                out_icon_size = fs::GetFileSize(DefaultHomebrewIconPath);
                if(out_icon_size <= 0) {
                    return false;
                }

                out_icon_buf = new u8[out_icon_size]();
                if(!fs::ReadFile(DefaultHomebrewIconPath, out_icon_buf, out_icon_size)) {
                    delete[] out_icon_buf;
                    return false;
                }
            }

            if(load_default_nacp) {
                const auto default_nacp_size = fs::GetFileSize(DefaultHomebrewNacpPath);
                if(default_nacp_size != sizeof(out_nacp)) {
                    return false;
                }

                if(!fs::ReadFile(DefaultHomebrewNacpPath, std::addressof(out_nacp), sizeof(out_nacp))) {
                    return false;
                }
            }

            return true;
        }

        void CacheApplicationEntry(const u64 app_id) {
            auto control_data = new NsApplicationControlData();
            if(R_SUCCEEDED(nsGetApplicationControlData(NsApplicationControlSource_Storage, app_id, control_data, sizeof(NsApplicationControlData), nullptr))) {
                const auto icon_path = GetApplicationCacheIcon(app_id);
                fs::WriteFile(icon_path, control_data->icon, sizeof(control_data->icon), true);

                const auto nacp_path = GetApplicationCacheNacp(app_id);
                fs::WriteFile(nacp_path, &control_data->nacp, sizeof(control_data->nacp), true);
            }
            delete control_data;
        }

        void CacheHomebrewEntry(const std::string &nro_path) {
            u8 *icon_buf;
            size_t icon_size;
            NacpStruct nacp;
            if(ReadHomebrewAssetInfo(nro_path, icon_buf, icon_size, nacp)) {
                const auto icon_path = GetHomebrewCacheIcon(nro_path);
                fs::WriteFile(icon_path, icon_buf, icon_size, true);

                const auto nacp_path = GetHomebrewCacheNacp(nro_path);
                fs::WriteFile(nacp_path, &nacp, sizeof(nacp), true);

                delete[] icon_buf;
            }
        }

        size_t CacheEntriesAt(const std::string &path, NsApplicationRecord *records_buf, const s32 record_count, u32 &out_cur_index, std::vector<std::string> &nro_paths) {
            size_t count = 0;
            UL_FS_FOR(path, entry_name, entry_path, is_dir, is_file, {
                if(is_file) {
                    const u32 idx = strtoul(entry_name.c_str(), nullptr, 10);
                    if(out_cur_index < idx) {
                        out_cur_index = idx;
                    }

                    std::string entry_json_str;
                    if(fs::ReadFileString(entry_path, entry_json_str)) {
                        const auto entry_json = nlohmann::json::parse(entry_json_str);

                        const auto raw_kind = entry_json.value("kind", "");
                        EntryKind kind;
                        if(TryParseEntryKind(raw_kind, kind)) {
                            switch(kind) {
                                case EntryKind::Application: {
                                    const auto raw_app_id = entry_json.value("application_id", "");
                                    const u64 app_id = strtoull(raw_app_id.c_str(), nullptr, 16);
                                    if(app_id != 0) {
                                        CacheApplicationEntry(app_id);
                                        for(s32 i = 0; i < record_count; i++) {
                                            if(records_buf[i].application_id == app_id) {
                                                records_buf[i].application_id = 0;
                                                break;
                                            }
                                        }
                                        count++;
                                    }
                                    break;
                                }
                                case EntryKind::Homebrew: {
                                    const auto nro_path = entry_json.value("nro_path", "");
                                    NroHeader dummy_header;
                                    if(IsValidHomebrew(nro_path, dummy_header)) {
                                        CacheHomebrewEntry(nro_path);
                                        util::VectorRemoveByValue(nro_paths, nro_path);
                                        count++;
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                    }
                }
                else if(is_dir) {
                    u32 cur_index = 0;
                    count += CacheEntriesAt(entry_path, records_buf, record_count, cur_index, nro_paths);
                }
            });
            return count;
        }

        void CreateApplicationEntry(const u64 app_id, u32 &cur_idx) {
            const auto name = std::to_string(cur_idx) + ".json";
            cur_idx++;
            const EntryPath path = {};
            const auto entry_json_path = ConvertEntryPath(path) + "/" + name;
            auto entry_json = nlohmann::json::object();
            entry_json["kind"] = "application";
            entry_json["application_id"] = util::FormatProgramId(app_id);

            const auto entry_json_str = entry_json.dump(4);
            if(fs::WriteFileString(entry_json_path, entry_json_str, true)) {
                CacheApplicationEntry(app_id);
            }
        }

        void CreateHomebrewEntry(const std::string &nro_path, u32 &cur_idx) {
            const auto name = std::to_string(cur_idx) + ".json";
            cur_idx++;
            const EntryPath path = {};
            const auto entry_json_path = ConvertEntryPath(path) + "/" + name;
            auto entry_json = nlohmann::json::object();
            entry_json["kind"] = "homebrew";
            entry_json["nro_path"] = nro_path;

            const auto entry_json_str = entry_json.dump(4);
            if(fs::WriteFileString(entry_json_path, entry_json_str, true)) {
                CacheHomebrewEntry(nro_path);
            }
        }

        constexpr size_t MaxApplicationCount = 64000;

    }

    void CacheEnsureEntries() {
        auto records_buf = new NsApplicationRecord[MaxApplicationCount]();
        s32 record_count;
        UL_RC_ASSERT(nsListApplicationRecord(records_buf, MaxApplicationCount, 0, &record_count));

        std::vector<std::string> nro_paths;
        // Manually add hbmenu as a special case
        nro_paths.push_back("sdmc:/hbmenu.nro");
        UL_FS_FOR("sdmc:/switch", nro_name, nro_path, is_dir, is_file, {
            if(is_file) {
                NroHeader dummy_header;
                if(IsValidHomebrew(nro_path, dummy_header)) {
                    nro_paths.push_back(nro_path);
                }
            }
        });

        u32 root_cur_index = 0;
        const auto cached_count = CacheEntriesAt(RootDirectory, records_buf, record_count, root_cur_index, nro_paths);
        
        // If anything was cache'd, then move to the next free index
        if(cached_count > 0) {
            root_cur_index++;
        }

        // Create new entries (are the only/valid ones left)

        for(s32 i = 0; i < record_count; i++) {
            const auto &record = records_buf[i];
            if(record.application_id != 0) {
                CreateApplicationEntry(record.application_id, root_cur_index);
            }
        }

        delete[] records_buf;

        for(const auto &nro_path: nro_paths) {
            CreateHomebrewEntry(nro_path, root_cur_index);
        }
    }

}