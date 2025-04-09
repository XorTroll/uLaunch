#include <ul/menu/menu_Entries.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_String.hpp>
#include <ul/util/util_Json.hpp>
#include <ul/os/os_Applications.hpp>
#include <ul/os/os_System.hpp>
#include <ul/ul_Result.hpp>
#include <ul/menu/menu_Cache.hpp>
#include <ul/acc/acc_Accounts.hpp>

namespace ul::menu {

    namespace {

        bool g_LoadApplicationEntryVersions = true;

        std::string g_ActiveMenuPath;

        inline u64 GetUpdateApplicationId(const u64 app_id) {
            return (app_id & ~0x800) | 0x800;
        }

        std::vector<NsApplicationRecord> g_ApplicationRecords = {};
        std::vector<os::ApplicationView> g_ApplicationViews = {};

        void LoadControlDataStrings(EntryControlData &out_control, NacpStruct *nacp) {
            NacpLanguageEntry *lang_entry = nullptr;
            const auto rc = nsGetApplicationDesiredLanguage(nacp, &lang_entry);
            if(R_FAILED(rc) || (lang_entry == nullptr)) {
                for(u32 i = 0; i < 16; i++) {
                    lang_entry = &nacp->lang[i];
                    if((lang_entry->name[0] > 0) && (lang_entry->author[0] > 0)) {
                        break;
                    }
                    lang_entry = nullptr;
                }
            }
            UL_ASSERT_TRUE(lang_entry != nullptr);

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

        void LoadHomebrewControlData(const std::string &nro_path, EntryControlData &out_control) {
            const auto cache_nacp_path = GetHomebrewCacheNacpPath(nro_path);
            if(fs::ExistsFile(cache_nacp_path)) {
                NacpStruct nacp = {};
                UL_ASSERT_TRUE(fs::ReadFile(cache_nacp_path, &nacp, sizeof(nacp)));
                LoadControlDataStrings(out_control, &nacp);
            }
        }

        void LoadApplicationControlData(const u64 app_id, EntryControlData &out_control) {
            auto tmp_control_data = new NsApplicationControlData();
            if(R_SUCCEEDED(nsGetApplicationControlData(NsApplicationControlSource_Storage, app_id, tmp_control_data, sizeof(NsApplicationControlData), nullptr))) {
                LoadControlDataStrings(out_control, &tmp_control_data->nacp);        
            }
        }

        inline void EnsureApplicationRecordsAndViews(const bool reload = false) {
            if(reload || g_ApplicationRecords.empty()) {
                g_ApplicationRecords = os::ListApplicationRecords();
                g_ApplicationViews = os::ListApplicationViews(g_ApplicationRecords);
            }
        }

        inline std::string MakeEntryPath(const std::string &base_path, const u32 idx) {
            return fs::JoinPath(base_path, std::to_string(idx) + ".m.json");
        }

        inline std::string MakeEntryFolderPath(const std::string &base_path, const std::string &name, const u32 name_idx) {
            return fs::JoinPath(base_path, "folder_" + name + "_" + std::to_string(name_idx));
        }

        u32 FindNextEntryIndex(const std::string &base_path) {
            u32 cur_idx = 0;
            while(fs::ExistsFile(MakeEntryPath(base_path, cur_idx))) {
                cur_idx++;
            }
            return cur_idx;
        }

        u32 FindNextFolderNameIndex(const std::string &base_path, const std::string &name) {
            u32 cur_idx = 0;
            while(fs::ExistsFile(MakeEntryFolderPath(base_path, name, cur_idx))) {
                cur_idx++;
            }
            return cur_idx;
        }

        std::string MakeAsciiString(const std::string &str) {
            std::string ascii_str;
            for(const auto c : str) {
                if(isascii(c)) {
                    ascii_str += c;
                }
            }
            return ascii_str;
        }

        inline std::string MakeNextFolderPath(const std::string &base_path, const std::string &folder_name) {
            // Ensure the filesystem folder name is ASCII for FS, while the actual folder name is saved in its JSON data
            auto ascii_name = MakeAsciiString(folder_name);
            if(ascii_name.empty()) {
                ascii_name = "null";
            }

            return MakeEntryFolderPath(base_path, ascii_name, FindNextFolderNameIndex(base_path, ascii_name));
        }

        std::string FindRootFolderPath(const std::string &folder_name) {
            UL_FS_FOR(V100_V110MenuPath, entry_name, entry_path, is_dir, is_file, {
                if(is_file && util::StringEndsWith(entry_name, ".m.json")) {
                    util::JSON folder_json;
                    if(R_SUCCEEDED(util::LoadJSONFromFile(folder_json, entry_path))) {
                        const auto type = static_cast<EntryType>(folder_json.value("type", static_cast<u32>(EntryType::Invalid)));
                        if(type == EntryType::Folder) {
                            const auto name = folder_json.value("name", "");
                            const auto fs_name = folder_json.value("fs_name", "");
                            if(folder_name == name) {
                                return fs::JoinPath(V100_V110MenuPath, fs_name);
                            }
                        }
                    }
                }
            });

            // Not existing, create it
            const auto new_folder_idx = FindNextEntryIndex(V100_V110MenuPath);
            const auto new_folder_entry = CreateFolderEntry(V100_V110MenuPath, folder_name, new_folder_idx);
            return fs::JoinPath(V100_V110MenuPath, new_folder_entry.folder_info.fs_name);
        }

        void InitializeRemainingEntries(const std::vector<NsApplicationRecord> &remaining_apps, u32 &entry_idx) {
            const std::vector<std::string> DefaultHomebrewRecordPaths = { HbmenuPath, ManagerPath };

            // Add special homebrew entries
            for(const auto &nro_path : DefaultHomebrewRecordPaths) {
                const Entry hb_entry = {
                    .type = EntryType::Homebrew,
                    .entry_path = MakeEntryPath(g_ActiveMenuPath, entry_idx),

                    .hb_info = {
                        .nro_target = loader::TargetInput::Create(nro_path, nro_path, true, "")
                    }
                };
                hb_entry.Save();
                entry_idx++;
            }

            // Add special uMenu entries
            #define _UL_MENU_ADD_SPECIAL_ENTRY(kind) { \
                const Entry special_entry = { \
                    .type = kind, \
                    .entry_path = MakeEntryPath(g_ActiveMenuPath, entry_idx) \
                }; \
                special_entry.Save(); \
                entry_idx++; \
            }
            _UL_MENU_ADD_SPECIAL_ENTRY(EntryType::SpecialEntryMiiEdit);
            _UL_MENU_ADD_SPECIAL_ENTRY(EntryType::SpecialEntryWebBrowser);
            _UL_MENU_ADD_SPECIAL_ENTRY(EntryType::SpecialEntryUserPage);
            _UL_MENU_ADD_SPECIAL_ENTRY(EntryType::SpecialEntrySettings);
            _UL_MENU_ADD_SPECIAL_ENTRY(EntryType::SpecialEntryThemes);
            _UL_MENU_ADD_SPECIAL_ENTRY(EntryType::SpecialEntryControllers);
            _UL_MENU_ADD_SPECIAL_ENTRY(EntryType::SpecialEntryAlbum);
            _UL_MENU_ADD_SPECIAL_ENTRY(EntryType::SpecialEntryAmiibo);

            // Add remaining app entries
            for(const auto &app_record : remaining_apps) {
                Entry app_entry = {
                    .type = EntryType::Application,
                    .entry_path = MakeEntryPath(g_ActiveMenuPath, entry_idx),

                    .app_info = {
                        .app_id = app_record.application_id,
                        .record = app_record
                    }
                };
                app_entry.Save();
                entry_idx++;
            }
        }

        void ConvertPreV100Menu(u32 &entry_idx) {
            if(fs::ExistsDirectory(PreV100MenuPath)) {
                if(!fs::ExistsDirectory(V100_V110MenuPath)) {
                    fs::CreateDirectory(V100_V110MenuPath);
                }

                EnsureApplicationRecordsAndViews();
                auto apps_copy = g_ApplicationRecords;

                UL_FS_FOR(PreV100MenuPath, old_entry_name, old_entry_path, is_dir, is_file, {
                    util::JSON old_entry_json;
                    if(R_SUCCEEDED(util::LoadJSONFromFile(old_entry_json, old_entry_path))) {
                        const auto type = static_cast<EntryType>(old_entry_json.value("type", static_cast<u32>(EntryType::Invalid)));
                        const auto folder_name = old_entry_json.value("folder", "");
                        const auto custom_name = old_entry_json.value("name", "");
                        const auto custom_author = old_entry_json.value("author", "");
                        const auto custom_version = old_entry_json.value("version", "");
                        const auto custom_icon_path = old_entry_json.value("icon_path", "");

                        std::string base_path = V100_V110MenuPath;
                        if(!folder_name.empty()) {
                            base_path = FindRootFolderPath(folder_name);
                        }

                        Entry entry = {
                            .type = type,
                            .entry_path = MakeEntryPath(base_path, entry_idx),
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
                        entry_idx++;

                        switch(type) {
                            case EntryType::Application: {
                                const auto application_id_fmt = old_entry_json.value("application_id", "");
                                const auto application_id = util::Get64FromString(application_id_fmt);
                                entry.app_info.app_id = application_id;

                                const auto find_rec = std::find_if(apps_copy.begin(), apps_copy.end(), [&](const NsApplicationRecord &rec) -> bool {
                                    return rec.application_id == application_id;
                                });
                                if(find_rec != apps_copy.end()) {
                                    entry.app_info.record = *find_rec;
                                    apps_copy.erase(find_rec);
                                    entry.Save();
                                }
                                else {
                                    UL_LOG_WARN("Found old pre-v1.0.0 menu application that could not be matched to an existing application record...");
                                }

                                const auto find_view = std::find_if(g_ApplicationViews.begin(), g_ApplicationViews.end(), [&](const os::ApplicationView &view) -> bool {
                                    return view.app_id == application_id;
                                });
                                if(find_view != g_ApplicationViews.end()) {
                                    entry.app_info.view = *find_view;
                                    entry.Save();
                                }
                                else {
                                    UL_LOG_WARN("Found old pre-v1.0.0 menu application that could not be matched to an existing application view...");
                                }

                                if(g_LoadApplicationEntryVersions) {
                                    auto rc = avmGetHighestAvailableVersion(application_id, GetUpdateApplicationId(application_id), &entry.app_info.version);
                                    if(R_FAILED(rc)) {
                                        entry.app_info.version = 0;
                                        UL_LOG_WARN("Found old pre-v1.0.0 menu application whose version could not be retrieved...");
                                    }
                                    rc = avmGetLaunchRequiredVersion(application_id, &entry.app_info.launch_required_version);
                                    if(R_FAILED(rc)) {
                                        entry.app_info.launch_required_version = 0;
                                        UL_LOG_WARN("Found old pre-v1.0.0 menu application whose launch required version could not be retrieved...");
                                    }
                                }
                                else {
                                    entry.app_info.version = 0;
                                    entry.app_info.launch_required_version = 0;
                                }
                                
                                break;
                            }
                            case EntryType::Homebrew: {
                                const auto nro_path = old_entry_json.value("nro_path", "");
                                const auto nro_argv = old_entry_json.value("nro_argv", "");

                                if(fs::ExistsFile(nro_path)) {
                                    entry.hb_info = {
                                        .nro_target = loader::TargetInput::Create(nro_path, nro_argv, true, "")
                                    };

                                    entry.Save();
                                }
                                else {
                                    UL_LOG_WARN("Found old pre-v1.0.0 menu homebrew entry whose NRO is not present...");
                                }
                                break;
                            }
                            default:
                                break;
                        }
                    }
                });

                InitializeRemainingEntries(apps_copy, entry_idx);

                fs::DeleteDirectory(PreV100MenuPath);
            }
        }

        void ConvertV100_V110Menu(const bool is_emummc) {
            if(!fs::ExistsDirectory(V100_V110MenuPath)) {
                return;
            }

            std::vector<AccountUid> uids;
            UL_RC_ASSERT(acc::ListAccounts(uids));

            for(const auto &uid : uids) {
                const auto menu_path = MakeMenuPath(is_emummc, uid);
                fs::DeleteDirectory(menu_path);

                UL_LOG_INFO("Copying old v1.0.0-v1.1.0 menu from '%s' to '%s'", V100_V110MenuPath, menu_path.c_str());
                fs::CopyDirectory(V100_V110MenuPath, menu_path);
            }

            fs::DeleteDirectory(V100_V110MenuPath);
        }

    }

    void Entry::TryLoadControlData() {
        if(!this->control.IsLoaded()) {
            switch(this->type) {
                case EntryType::Application: {
                    LoadApplicationControlData(this->app_info.app_id, this->control);
                    break;
                }
                case EntryType::Homebrew: {
                    LoadHomebrewControlData(this->hb_info.nro_target.nro_path, this->control);
                    break;
                }
                default:
                    // Folders and special entries do not use control data
                    break;
            }
        }
    }

    void Entry::ReloadApplicationInfo(const bool force_reload_records_views) {
        if(this->Is<EntryType::Application>()) {
            const auto app_id = this->app_info.app_id;

            // Assume we need to reload here
            EnsureApplicationRecordsAndViews(force_reload_records_views);

            const auto find_rec = std::find_if(g_ApplicationRecords.begin(), g_ApplicationRecords.end(), [&](const NsApplicationRecord &rec) -> bool {
                return rec.application_id == app_id;
            });
            if(find_rec != g_ApplicationRecords.end()) {
                this->app_info.record = *find_rec;
            }
            else {
                UL_LOG_WARN("Unable to reload application record: not found?");
            }

            const auto find_view = std::find_if(g_ApplicationViews.begin(), g_ApplicationViews.end(), [&](const os::ApplicationView &view) -> bool {
                return view.app_id == app_id;
            });
            if(find_view != g_ApplicationViews.end()) {
                this->app_info.view = *find_view;
            }
            else {
                UL_LOG_WARN("Unable to reload application view: not found?");
            }
        }
    }

    void Entry::MoveTo(const std::string &new_folder_path) {
        // Must deal with folder renaming first, since the general moving code below will modify the folder path
        if(this->Is<EntryType::Folder>()) {
            const auto old_fs_path = this->GetFolderPath();
            const auto new_fs_path = MakeNextFolderPath(new_folder_path, this->folder_info.name);
            util::CopyToStringBuffer(this->folder_info.fs_name, fs::GetBaseName(new_fs_path));
            fs::RenameDirectory(old_fs_path, new_fs_path);
        }

        const auto new_idx = FindNextEntryIndex(new_folder_path);
        this->index = new_idx;
        const auto new_entry_path = MakeEntryPath(new_folder_path, new_idx);
        fs::RenameFile(this->entry_path, new_entry_path);
        this->entry_path = new_entry_path;

        this->Save();
    }

    bool Entry::MoveToIndex(const u32 new_index) {
        const auto cur_folder_path = fs::GetBaseDirectory(this->entry_path);
        const auto new_entry_path = MakeEntryPath(cur_folder_path, new_index);
        if(fs::ExistsFile(new_entry_path)) {
            return false;
        }

        fs::RenameFile(this->entry_path, new_entry_path);
        this->entry_path = new_entry_path;
        this->index = new_index;
        return true;
    }

    void Entry::OrderSwap(Entry &other_entry) {
        const auto tmp_entry_path = other_entry.entry_path + ".tmp";
        fs::RenameDirectory(other_entry.entry_path, tmp_entry_path);
        fs::RenameDirectory(this->entry_path, other_entry.entry_path);
        fs::RenameDirectory(tmp_entry_path, this->entry_path);

        std::swap(this->entry_path, other_entry.entry_path);
        std::swap(this->index, other_entry.index);

        this->Save();
        other_entry.Save();
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
                entry_json["application_id"] = this->app_info.app_id;
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

    std::vector<Entry> Entry::Remove() {
        fs::DeleteFile(this->entry_path);

        if(this->Is<EntryType::Folder>()) {
            const auto fs_path = this->GetFolderPath();

            auto folder_entries = LoadEntries(fs_path);
            for(auto &entry: folder_entries) {
                entry.MoveToParentFolder();
            }

            fs::DeleteDirectory(fs_path);

            // Returns new entries present in the path where the entry (the folder actually) was removed
            return folder_entries;
        }

        return {};
    }

    void SetLoadApplicationEntryVersions(const bool load) {
        g_LoadApplicationEntryVersions = load;
    }

    void InitializeEntries(const bool is_emummc, const AccountUid &uid) {
        u32 entry_idx = 0;
        g_ActiveMenuPath = MakeMenuPath(is_emummc, uid);

        EnsureApplicationRecordsAndViews();

        ConvertPreV100Menu(entry_idx);
        ConvertV100_V110Menu(is_emummc);

        if(!fs::ExistsDirectory(g_ActiveMenuPath)) {
            fs::CreateDirectory(g_ActiveMenuPath);

            InitializeRemainingEntries(g_ApplicationRecords, entry_idx);
        }
    }

    std::vector<Entry> LoadEntries(const std::string &path) {
        EnsureApplicationRecordsAndViews();

        std::vector<Entry> entries;
        UL_FS_FOR(path, entry_name, entry_path, is_dir, is_file, {
            if(is_file && util::StringEndsWith(entry_name, ".m.json")) {
                util::JSON entry_json;
                if(R_SUCCEEDED(util::LoadJSONFromFile(entry_json, entry_path))) {
                    const auto type = static_cast<EntryType>(entry_json.value("type", static_cast<u32>(EntryType::Invalid)));
                    const auto entry_index_str = entry_name.substr(0, entry_name.length() - __builtin_strlen(".m.json"));
                    const u32 entry_index = std::strtoul(entry_index_str.c_str(), nullptr, 10);
                    const auto custom_name_str = entry_json.value("custom_name", "");
                    const auto custom_author_str = entry_json.value("custom_author", "");
                    const auto custom_version_str = entry_json.value("custom_version", "");
                    const auto custom_icon_path_str = entry_json.value("custom_icon_path", "");

                    Entry entry = {
                        .type = type,
                        .entry_path = entry_path,
                        .index = entry_index,

                        .control = {
                            .name = custom_name_str,
                            .custom_name = !custom_name_str.empty(),
                            .author = custom_author_str,
                            .custom_author = !custom_author_str.empty(),
                            .version = custom_version_str,
                            .custom_version = !custom_version_str.empty(),
                            .icon_path = custom_icon_path_str,
                            .custom_icon_path = !custom_icon_path_str.empty(),
                        }
                    };

                    switch(type) {
                        case EntryType::Application: {
                            const auto application_id = entry_json.value("application_id", static_cast<u64>(0));

                            entry.app_info = {
                                .app_id = application_id
                            };

                            const auto find_rec = std::find_if(g_ApplicationRecords.begin(), g_ApplicationRecords.end(), [&](const NsApplicationRecord &rec) -> bool {
                                return rec.application_id == application_id;
                            });
                            if(find_rec != g_ApplicationRecords.end()) {
                                entry.app_info.record = *find_rec;
                            }
                            else {
                                UL_LOG_WARN("Potentially invalid application entry: unable to match to application record");
                            }

                            const auto find_view = std::find_if(g_ApplicationViews.begin(), g_ApplicationViews.end(), [&](const os::ApplicationView &view) -> bool {
                                return view.app_id == application_id;
                            });
                            if(find_view != g_ApplicationViews.end()) {
                                entry.app_info.view = *find_view;
                            }
                            else {
                                UL_LOG_WARN("Potentially invalid application entry: unable to match to application view");
                            }

                            if(g_LoadApplicationEntryVersions) {
                                auto rc = avmGetHighestAvailableVersion(application_id, GetUpdateApplicationId(application_id), &entry.app_info.version);
                                if(R_FAILED(rc)) {
                                    entry.app_info.version = 0;
                                    UL_LOG_WARN("Potentially invalid application entry: unable to retrieve version");
                                }
                                rc = avmGetLaunchRequiredVersion(application_id, &entry.app_info.launch_required_version);
                                if(R_FAILED(rc)) {
                                    entry.app_info.launch_required_version = 0;
                                    UL_LOG_WARN("Potentially invalid application entry: unable to retrieve launch required version");
                                }
                            }
                            else {
                                entry.app_info.version = 0;
                                entry.app_info.launch_required_version = 0;
                            }

                            entries.push_back(entry);
                            break;
                        }
                        case EntryType::Homebrew: {
                            const auto nro_path = entry_json.value("nro_path", "");
                            const auto nro_argv = entry_json.value("nro_argv", "");

                            if(!entry.control.custom_icon_path) {
                                // Only set the icon if it's valid
                                const auto cache_icon_path = GetHomebrewCacheIconPath(nro_path);
                                if(!fs::ExistsFile(cache_icon_path)) {
                                    // If the homebrew changed before the system reboots, we need to cache it again
                                    CacheHomebrewEntry(nro_path);
                                }
                                
                                if(fs::ExistsFile(cache_icon_path)) {
                                    entry.control.icon_path = cache_icon_path;   
                                }
                                else {
                                    UL_LOG_WARN("Unable to cache homebrew entry: '%s'", nro_path.c_str());
                                }
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
                                UL_LOG_WARN("Invalid folder entry with empty name");
                            }
                            else if(fs_name.empty()) {
                                UL_LOG_WARN("Invalid folder entry with empty filesystem-name");
                            }
                            else {
                                entry.folder_info = {};
                                util::CopyToStringBuffer(entry.folder_info.name, name);
                                util::CopyToStringBuffer(entry.folder_info.fs_name, fs_name);

                                entries.push_back(entry);
                            }
                            break;
                        }
                        case EntryType::SpecialEntryMiiEdit:
                        case EntryType::SpecialEntryWebBrowser:
                        case EntryType::SpecialEntryUserPage:
                        case EntryType::SpecialEntrySettings:
                        case EntryType::SpecialEntryThemes:
                        case EntryType::SpecialEntryControllers:
                        case EntryType::SpecialEntryAlbum:
                        case EntryType::SpecialEntryAmiibo:
                            entries.push_back(entry);
                            break;
                        default:
                            break;
                    }
                }
            }
        });

        return entries;
    }

    const std::string &GetActiveMenuPath() {
        return g_ActiveMenuPath;
    }

    void EnsureApplicationEntry(const NsApplicationRecord &app_record) {
        // Just fill enough fields needed to save the path
        const auto entry_idx = FindNextEntryIndex(g_ActiveMenuPath);
        Entry app_entry = {
            .type = EntryType::Application,
            .entry_path = MakeEntryPath(g_ActiveMenuPath, entry_idx),

            .app_info = {
                .app_id = app_record.application_id,
                .record = app_record
            }
        };
        app_entry.Save();
    }

    Entry CreateFolderEntry(const std::string &base_path, const std::string &folder_name, const s32 index) {
        u32 actual_index;
        if(index < 0) {
            actual_index = FindNextEntryIndex(base_path);
        }
        else {
            actual_index = (u32)index;
        }

        const auto folder_path = MakeNextFolderPath(base_path, folder_name);
        fs::CreateDirectory(folder_path);

        Entry folder_entry = {
            .type = EntryType::Folder,
            .entry_path = MakeEntryPath(base_path, actual_index),
            .index = actual_index,

            .control = {
                .custom_name = false,
                .custom_author = false,
                .custom_version = false,
                .custom_icon_path = false
            },

            .folder_info = {}
        };
        util::CopyToStringBuffer(folder_entry.folder_info.name, folder_name);
        util::CopyToStringBuffer(folder_entry.folder_info.fs_name, fs::GetBaseName(folder_path));

        folder_entry.Save();
        return folder_entry;
    }

    Entry CreateHomebrewEntry(const std::string &base_path, const std::string &nro_path, const std::string &nro_argv, const s32 index) {
        u32 actual_index;
        if(index < 0) {
            actual_index = FindNextEntryIndex(base_path);
        }
        else {
            actual_index = (u32)index;
        }

        Entry hb_entry = {
            .type = EntryType::Homebrew,
            .entry_path = MakeEntryPath(base_path, actual_index),
            .index = actual_index,

            .control = {
                .custom_name = false,
                .custom_author = false,
                .custom_version = false,
                .custom_icon_path = false
            },

            .hb_info = {
                .nro_target = loader::TargetInput::Create(nro_path, nro_argv, true, "")
            }
        };

        hb_entry.TryLoadControlData();

        // Only set the icon if it's valid
        const auto cache_icon_path = GetHomebrewCacheIconPath(nro_path);
        if(fs::ExistsFile(cache_icon_path)) {
            hb_entry.control.icon_path = cache_icon_path;
        }

        hb_entry.Save();
        return hb_entry;
    }

    Entry CreateSpecialEntry(const std::string &base_path, const EntryType type, const s32 index) {
        u32 actual_index;
        if(index < 0) {
            actual_index = FindNextEntryIndex(base_path);
        }
        else {
            actual_index = (u32)index;
        }

        Entry special_entry = {
            .type = type,
            .entry_path = MakeEntryPath(base_path, actual_index),
            .index = actual_index
        };

        special_entry.Save();
        return special_entry;
    }

    void DeleteApplicationEntryRecursively(const u64 app_id, const std::string &path) {
        auto cur_entries = LoadEntries(path);
        for(auto &entry: cur_entries) {
            if(entry.Is<EntryType::Application>() && (entry.app_info.app_id == app_id)) {
                entry.Remove();
            }
            else if(entry.Is<EntryType::Folder>()) {
                const auto new_path = fs::JoinPath(path, entry.folder_info.fs_name);
                DeleteApplicationEntryRecursively(app_id, new_path);
            }
        }
    }

    void ReloadApplicationEntryInfos(std::vector<Entry> &entries) {
        // Helper to only reload records/views once
        EnsureApplicationRecordsAndViews(true);

        for(auto &entry: entries) {
            entry.ReloadApplicationInfo(false);
        }
    }

}
