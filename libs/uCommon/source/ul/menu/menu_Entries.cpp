#include <ul/menu/menu_Entries.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_String.hpp>
#include <ul/util/util_Json.hpp>
#include <ul/os/os_Applications.hpp>
#include <ul/ul_Result.hpp>
#include <ul/menu/menu_Cache.hpp>

namespace ul::menu {

    namespace {

        std::vector<NsApplicationRecord> g_ApplicationRecords = {};
        NacpStruct g_DefaultHomebrewNacp = {};

        void LoadControlDataStrings(EntryControlData &out_control, NacpStruct *nacp) {
            NacpLanguageEntry *lang_entry = nullptr;
            nacpGetLanguageEntry(nacp, &lang_entry);
            if(lang_entry == nullptr) {
                for(u32 i = 0; i < 16; i++) {
                    lang_entry = &nacp->lang[i];
                    if((lang_entry->name[0] > 0) && (lang_entry->author[0] > 0)) {
                        break;
                    }
                    lang_entry = nullptr;
                }
            }

            if(lang_entry != nullptr) {
                if(!out_control.custom_name) {
                    out_control.name = lang_entry->name;
                }
                if(!out_control.custom_author) {
                    out_control.author = lang_entry->author;
                }
                if(!out_control.custom_version) {
                    out_control.version = nacp->display_version;
                }
            }
        }

        void EnsureLoadDefaultHomebrewNacp() {
            if(g_DefaultHomebrewNacp.display_version[0] == 0) {
                UL_ASSERT_TRUE(fs::ReadFile(DefaultHomebrewNacpPath, &g_DefaultHomebrewNacp, sizeof(g_DefaultHomebrewNacp)));
            }
        }

        void LoadHomebrewControlData(const std::string &nro_path, EntryControlData &out_control) {
            auto loaded = false;

            const auto cache_nacp_path = GetHomebrewCacheNacpPath(nro_path);
            if(fs::ExistsFile(cache_nacp_path)) {
                NacpStruct nacp = {};
                UL_ASSERT_TRUE(fs::ReadFile(cache_nacp_path, &nacp, sizeof(nacp)));
                LoadControlDataStrings(out_control, &nacp);
                loaded = true;
            }
            if(!loaded) {
                // Default NACP strings
                EnsureLoadDefaultHomebrewNacp();
                LoadControlDataStrings(out_control, &g_DefaultHomebrewNacp);
            }
        }

        void LoadApplicationControlData(const u64 app_id, EntryControlData &out_control) {
            auto tmp_control_data = new NsApplicationControlData();
            if(R_SUCCEEDED(nsGetApplicationControlData(NsApplicationControlSource_Storage, app_id, tmp_control_data, sizeof(NsApplicationControlData), nullptr))) {
                LoadControlDataStrings(out_control, &tmp_control_data->nacp);        
            }
            else {
                // TODONEW: proper default NACP strings?
                EnsureLoadDefaultHomebrewNacp();
                LoadControlDataStrings(out_control, &g_DefaultHomebrewNacp);
            }
        }

        void EnsureApplicationRecords() {
            if(g_ApplicationRecords.empty()) {
                g_ApplicationRecords = os::ListApplicationRecords();
            }
        }

        std::string FindRootFolderPath(const std::string &folder_name) {
            UL_FS_FOR(MenuPath, entry_name, entry_path, is_dir, is_file, {
                if(is_file && util::StringEndsWith(entry_name, ".m.json")) {
                    util::JSON folder_json;
                    if(R_SUCCEEDED(util::LoadJSONFromFile(folder_json, entry_path))) {
                        const auto type = static_cast<EntryType>(folder_json.value("type", static_cast<u32>(EntryType::Invalid)));
                        if(type == EntryType::Folder) {
                            const auto name = folder_json.value("name", "");
                            const auto fs_name = folder_json.value("fs_name", "");
                            if(folder_name == name) {
                                return fs::JoinPath(MenuPath, fs_name);
                            }
                        }
                    }
                }
            });

            // Not existing, create it
            const auto new_folder_entry = CreateFolderEntry(MenuPath, folder_name);
            return fs::JoinPath(MenuPath, new_folder_entry.folder_info.fs_name);
        }

        u32 RandomFromRange(const u32 min, const u32 max) {
            const auto diff = max - min;
            u32 random_val;
            do {
                randomGet(&random_val, sizeof(random_val));
                random_val %= (diff + 1);
                random_val += min;
            } while((random_val == min) || (random_val == max));
            return random_val;
        }

        std::string AllocateEntryPath(const u32 start_idx, const u32 end_idx, const std::string &base_path, u32 &out_idx) {
            u32 idx;
            std::string test_path;
            do {
                idx = RandomFromRange((start_idx == InvalidEntryIndex) ? 0 : start_idx, end_idx);
                test_path = fs::JoinPath(base_path, std::to_string(idx) + ".m.json");
            } while(fs::ExistsFile(test_path));
            out_idx = idx;
            return test_path;
        }

        // 2 48 49 50 450

        void ConvertOldMenu() {
            if(fs::ExistsDirectory(OldMenuPath)) {
                if(!fs::ExistsDirectory(MenuPath)) {
                    fs::CreateDirectory(MenuPath);
                }

                UL_FS_FOR(OldMenuPath, old_entry_name, old_entry_path, is_dir, is_file, {
                    util::JSON old_entry_json;
                    if(R_SUCCEEDED(util::LoadJSONFromFile(old_entry_json, old_entry_path))) {
                        const auto type = static_cast<EntryType>(old_entry_json.value("type", static_cast<u32>(EntryType::Invalid)));
                        const auto folder_name = old_entry_json.value("folder", "");
                        const auto custom_name = old_entry_json.value("name", "");
                        const auto custom_author = old_entry_json.value("author", "");
                        const auto custom_version = old_entry_json.value("version", "");
                        const auto custom_icon_path = old_entry_json.value("icon_path", "");

                        std::string base_path = MenuPath;
                        if(!folder_name.empty()) {
                            base_path = FindRootFolderPath(folder_name);
                        }

                        u32 tmp_idx;
                        Entry entry = {
                            .type = type,
                            .entry_path = AllocateEntryPath(InvalidEntryIndex, InvalidEntryIndex, base_path, tmp_idx),
                            .index = 0,

                            .control = {
                                .name = custom_name,
                                .custom_name = !custom_name.empty(),
                                .author = custom_author,
                                .custom_author = !custom_name.empty(),
                                .version = custom_version,
                                .custom_version = !custom_name.empty(),
                                .icon_path = custom_icon_path,
                                .custom_icon_path = !custom_name.empty(),
                            }
                        };
                        entry.index = tmp_idx;

                        switch(type) {
                            case EntryType::Application: {
                                const auto application_id_fmt = old_entry_json.value("application_id", "");
                                const auto application_id = util::Get64FromString(application_id_fmt);

                                const auto find_rec = std::find_if(g_ApplicationRecords.begin(), g_ApplicationRecords.end(), [&](const NsApplicationRecord &rec) -> bool {
                                    return rec.application_id == application_id;
                                });
                                if(find_rec != g_ApplicationRecords.end()) {
                                    entry.app_info.record = *find_rec;
                                }
                                else {
                                    // TODONEW: logging system, log warns/errors!
                                }

                                entry.Save();
                                break;
                            }
                            case EntryType::Homebrew: {
                                const auto nro_path = old_entry_json.value("nro_path", "");
                                const auto nro_argv = old_entry_json.value("nro_argv", "");

                                entry.hb_info = {
                                    .nro_target = loader::TargetInput::Create(nro_path, nro_argv, true, "")
                                };

                                entry.Save();
                                break;
                            }
                            default:
                                break;
                        }
                    }
                });

                fs::DeleteDirectory(OldMenuPath);
            }
        }

    }

    void Entry::TryLoadControlData() {
        if(!this->control.IsLoaded()) {
            switch(this->type) {
                case EntryType::Application: {
                    LoadApplicationControlData(this->app_info.record.application_id, this->control);
                    break;
                }
                case EntryType::Homebrew: {
                    LoadHomebrewControlData(this->hb_info.nro_target.nro_path, this->control);
                    break;
                }
                case EntryType::Folder: {
                    // Folders do not use control data
                    break;
                }
                default:
                    break;
            }
        }
    }

    void Entry::MoveTo(const std::string &new_folder_path) {
        if(this->Is<EntryType::Folder>()) {
            const auto old_fs_path = this->GetFolderPath();
            const auto new_fs_path = fs::JoinPath(new_folder_path, this->folder_info.fs_name);
            fs::RenameDirectory(old_fs_path, new_fs_path);
        }

        const auto new_entry_path = fs::JoinPath(new_folder_path, fs::GetBaseName(this->entry_path));
        fs::RenameFile(this->entry_path, new_entry_path);
        this->entry_path = new_entry_path;
    }

    void Entry::Save() const {
        auto entry_json = util::JSON::object();
        entry_json["type"] = static_cast<u32>(this->type);

        if(this->control.custom_name) {
            entry_json["custom_name"] = this->control.name;
        }
        if(this->control.custom_author) {
            entry_json["custom_author"] = this->control.author;
        }
        if(this->control.custom_version) {
            entry_json["custom_version"] = this->control.version;
        }
        if(this->control.custom_icon_path) {
            entry_json["custom_icon_path"] = this->control.icon_path;
        }

        switch(this->type) {
            case EntryType::Application: {
                entry_json["application_id"] = this->app_info.record.application_id;
                break;
            }
            case EntryType::Homebrew: {
                entry_json["nro_path"] = this->hb_info.nro_target.nro_path;
                entry_json["nro_argv"] = this->hb_info.nro_target.nro_argv;
                break;
            }
            case EntryType::Folder: {
                entry_json["name"] = this->folder_info.name;
                entry_json["fs_name"] = this->folder_info.fs_name;
                break;
            }
            default:
                break;
        }

        util::SaveJSON(this->entry_path, entry_json);
    }

    void Entry::Remove() {
        if(this->Is<EntryType::Folder>()) {
            // Move all the items outside
            const auto fs_path = this->GetFolderPath();
            const auto parent_path = fs::GetBaseDirectory(fs_path);
            fs::RenameDirectory(fs_path, parent_path);
        }
    
        fs::DeleteFile(this->entry_path);
    }

    void Entry::OrderBetween(const u32 start_idx, const u32 end_idx) {
        const auto new_entry_path = AllocateEntryPath(start_idx, end_idx, fs::GetBaseDirectory(this->entry_path), this->index);
        fs::RenameFile(this->entry_path, new_entry_path);
        this->entry_path = new_entry_path;
    }

    void Entry::OrderSwap(Entry &other_entry) {
        const auto tmp_entry_path = other_entry.entry_path + ".tmp";
        fs::RenameDirectory(other_entry.entry_path, tmp_entry_path);
        fs::RenameDirectory(this->entry_path, other_entry.entry_path);
        fs::RenameDirectory(tmp_entry_path, this->entry_path);

        std::swap(this->entry_path, other_entry.entry_path);
        std::swap(this->index, other_entry.index);
    }

    void InitializeEntries() {
        EnsureApplicationRecords();

        ConvertOldMenu();

        if(!fs::ExistsDirectory(MenuPath)) {
            fs::CreateDirectory(MenuPath);

            // Reserve for all apps + hbmenu
            const auto index_gap = UINT32_MAX / (g_ApplicationRecords.size() + 1);
            u32 cur_start_idx = 0;
            for(const auto &app_record : g_ApplicationRecords) {
                EnsureApplicationEntry(app_record, cur_start_idx, cur_start_idx + index_gap);
                cur_start_idx += index_gap;
            }

            // Add hbmenu too
            u32 tmp_idx;
            const Entry hbmenu_entry = {
                .type = EntryType::Homebrew,
                .entry_path = AllocateEntryPath(cur_start_idx, cur_start_idx + index_gap, MenuPath, tmp_idx),

                .hb_info = {
                    .nro_target = loader::TargetInput::Create(HbmenuPath, HbmenuPath, true, "")
                }
            };
            hbmenu_entry.Save();
            // cur_start_idx += index_gap;
        }
    }

    void EnsureApplicationEntry(const NsApplicationRecord &app_record, const u32 start_idx, const u32 end_idx) {
        // Just fill enough fields needed to save the path
        u32 tmp_idx;
        Entry app_entry = {
            .type = EntryType::Application,
            .entry_path = AllocateEntryPath(start_idx, end_idx, MenuPath, tmp_idx),

            .app_info = {
                .record = app_record
            }
        };
        app_entry.Save();
    }

    std::vector<Entry> LoadEntries(const std::string &path) {
        std::vector<Entry> entries;
        UL_FS_FOR(path, entry_name, entry_path, is_dir, is_file, {
            if(is_file && util::StringEndsWith(entry_name, ".m.json")) {
                util::JSON entry_json;
                if(R_SUCCEEDED(util::LoadJSONFromFile(entry_json, entry_path))) {
                    const auto type = static_cast<EntryType>(entry_json.value("type", static_cast<u32>(EntryType::Invalid)));
                    const auto entry_index_str = entry_name.substr(0, entry_name.length() - __builtin_strlen(".m.json"));
                    const u32 entry_index = std::strtoul(entry_index_str.c_str(), nullptr, 10);
                    const auto custom_name = entry_json.value("custom_name", "");
                    const auto custom_author = entry_json.value("custom_author", "");
                    const auto custom_version = entry_json.value("custom_version", "");
                    const auto custom_icon_path = entry_json.value("custom_icon_path", "");

                    Entry entry = {
                        .type = type,
                        .entry_path = entry_path,
                        .index = entry_index,

                        .control = {
                            .name = custom_name,
                            .custom_name = !custom_name.empty(),
                            .author = custom_author,
                            .custom_author = !custom_name.empty(),
                            .version = custom_version,
                            .custom_version = !custom_name.empty(),
                            .icon_path = custom_icon_path,
                            .custom_icon_path = !custom_name.empty(),
                        }
                    };

                    switch(type) {
                        case EntryType::Application: {
                            const auto application_id = entry_json.value("application_id", static_cast<u64>(0));

                            if(!entry.control.custom_icon_path) {
                                entry.control.icon_path = GetApplicationCacheIconPath(application_id);
                            }

                            entry.app_info = {
                                .meta_status = os::GetApplicationContentMetaStatus(application_id)
                            };

                            const auto find_rec = std::find_if(g_ApplicationRecords.begin(), g_ApplicationRecords.end(), [&](const NsApplicationRecord &rec) -> bool {
                                return rec.application_id == application_id;
                            });
                            if(find_rec != g_ApplicationRecords.end()) {
                                entry.app_info.record = *find_rec;
                            }
                            else {
                                UL_LOG_WARN("invalid app entry with no record");
                            }

                            entries.push_back(entry);
                            break;
                        }
                        case EntryType::Homebrew: {
                            const auto nro_path = entry_json.value("nro_path", "");
                            const auto nro_argv = entry_json.value("nro_argv", "");

                            if(!entry.control.custom_icon_path) {
                                entry.control.icon_path = GetHomebrewCacheIconPath(nro_path);
                            }

                            entry.hb_info = {
                                .nro_target = loader::TargetInput::Create(nro_path, nro_argv, true, "")
                            };

                            entries.push_back(entry);
                            break;
                        }
                        case EntryType::Folder: {
                            const auto name = entry_json.value("name", "");
                            const auto fs_name = entry_json.value("fs_name", "");

                            if(name.empty()) {
                                UL_LOG_WARN("invalid folder entry with empty name");
                            }
                            else if(fs_name.empty()) {
                                UL_LOG_WARN("invalid folder entry with empty fs name");
                            }
                            else {
                                entry.folder_info = {};
                                util::CopyToStringBuffer(entry.folder_info.name, name);
                                util::CopyToStringBuffer(entry.folder_info.fs_name, fs_name);

                                entries.push_back(entry);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        });

        std::sort(entries.begin(), entries.end());

        auto needs_reindexing = false;
        for(u32 i = 0; i < entries.size(); i++) {
            auto &cur_entry = entries.at(i);
            if(i < (entries.size() - 1)) {
                auto &next_entry = entries.at(i + 1);

                if((next_entry.index - cur_entry.index) == 1) {
                    needs_reindexing = true;
                    break;
                }
            }
        }

        if(needs_reindexing) {
            const auto index_gap = UINT32_MAX / entries.size();
            u32 cur_start_idx = 0;
            for(auto &entry : entries) {
                u32 new_idx;
                entry.entry_path = AllocateEntryPath(cur_start_idx, cur_start_idx + index_gap, path, new_idx);
                entry.index = new_idx;
                entry.Save();
                cur_start_idx += index_gap;
            }
        }

        return entries;
    }

    Entry CreateFolderEntry(const std::string &base_path, const std::string &folder_name) {
        // Ensure the filesystem folder name is ASCII for FS, while the actual folder name is saved in its JSON data
        std::string folder_name_path = "folder";
        for(const auto folder_name_c : folder_name) {
            if(isascii(folder_name_c)) {
                folder_name_path += folder_name_c;
            }
        }
        
        const auto folder_path = fs::JoinPath(base_path, folder_name_path);
        fs::CreateDirectory(folder_path);

        u32 tmp_idx;
        Entry folder_entry = {
            .type = EntryType::Folder,
            .entry_path = AllocateEntryPath(InvalidEntryIndex, InvalidEntryIndex, base_path, tmp_idx),

            .control = {
                .custom_name = false,
                .custom_author = false,
                .custom_version = false,
                .custom_icon_path = false
            },

            .folder_info = {}
        };
        util::CopyToStringBuffer(folder_entry.folder_info.name, folder_name);
        util::CopyToStringBuffer(folder_entry.folder_info.fs_name, folder_name_path);

        folder_entry.Save();
        return folder_entry;
    }

    Entry CreateHomebrewEntry(const std::string &base_path, const std::string &nro_path, const std::string &nro_argv) {
        u32 tmp_idx;
        const Entry hb_entry = {
            .type = EntryType::Homebrew,
            .entry_path = AllocateEntryPath(InvalidEntryIndex, InvalidEntryIndex, base_path, tmp_idx),

            .hb_info = {
                .nro_target = loader::TargetInput::Create(nro_path, nro_argv, true, "")
            }
        };

        hb_entry.Save();
        return hb_entry;
    }

}