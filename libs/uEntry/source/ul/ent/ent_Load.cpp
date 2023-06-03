#include <ul/ent/ent_Load.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <extras/json.hpp>

namespace ul::ent {

    namespace {

        bool TryLoadEntry(const std::string &base_path, const std::string &json_name, Entry &out_entry) {
            try {
                const auto path = base_path + "/" + json_name + ".json";
                std::string entry_json_str;
                if(!fs::ReadFileString(path, entry_json_str)) {
                    return false;
                }

                const auto entry_json = nlohmann::json::parse(entry_json_str);

                const auto raw_kind = entry_json.value("kind", "");
                if(!TryParseEntryKind(raw_kind, out_entry.kind)) {
                    return false;
                }

                const auto index = strtoul(json_name.c_str(), nullptr, 10);
                out_entry.index = index;

                std::string cache_icon_path;
                std::string cache_nacp_path;
                switch(out_entry.kind) {
                    case EntryKind::Application: {
                        const auto raw_app_id = entry_json.value("application_id", "");
                        const auto app_id = strtoull(raw_app_id.c_str(), nullptr, 16);
                        if(app_id == 0) {
                            return false;
                        }

                        cache_icon_path = GetApplicationCacheIcon(app_id);
                        cache_nacp_path = GetApplicationCacheNacp(app_id);
                        out_entry.metadata.app_metadata = {
                            .app_id = app_id
                        };
                        break;
                    }
                    case EntryKind::Homebrew: {
                        const auto nro_path = entry_json.value("nro_path", "");
                        if(!fs::ExistsFile(nro_path)) {
                            return false;
                        }
                        auto nro_argv = entry_json.value("nro_argv", "");
                        if(nro_argv.empty()) {
                            nro_argv = nro_path;
                        }

                        cache_icon_path = GetHomebrewCacheIcon(nro_path);
                        cache_nacp_path = GetHomebrewCacheNacp(nro_path);
                        out_entry.metadata.hb_metadata = {
                            .nro_path = nro_path,
                            .nro_argv = nro_argv
                        };
                        break;
                    }
                    case EntryKind::Folder: {
                        const auto folder_name = entry_json.value("name", "");
                        if(!fs::ExistsDirectory(base_path + "/" + folder_name)) {
                            return false;
                        }

                        out_entry.metadata.name = folder_name;
                        // Nothing else to do with folder entries
                        return true;
                    }
                    default:
                        return false;
                }
                out_entry.metadata.icon_path = cache_icon_path;

                NacpStruct nacp;
                if(!fs::ReadFile(cache_nacp_path, &nacp, sizeof(nacp))) {
                    return false;
                }
                if(!out_entry.metadata.SetFromNacp(nacp)) {
                    return false;
                }

                const auto custom_name = entry_json.value("name", "");
                if(!custom_name.empty()) {
                    out_entry.metadata.name = custom_name;
                }
                const auto custom_author = entry_json.value("author", "");
                if(!custom_author.empty()) {
                    out_entry.metadata.author = custom_author;
                }
                const auto custom_version = entry_json.value("version", "");
                if(!custom_version.empty()) {
                    out_entry.metadata.version = custom_version;
                }

                return true;
            }
            catch(std::exception&) {
                return false;
            }
        }

    }

    void LoadEntries(const EntryPath &path, std::vector<Entry> &out_entries) {
        out_entries.clear();

        const auto actual_path = ConvertEntryPath(path);
        UL_FS_FOR(actual_path, entry_name, entry_path, is_dir, is_file, {
            if(is_file && util::StringEndsWith(entry_name, ".json")) {
                const auto json_name = entry_name.substr(0, entry_name.length() - 5);
                Entry entry;
                if(TryLoadEntry(actual_path, json_name, entry)) {
                    entry.path = path;
                    entry.name = entry_name;
                    out_entries.push_back(entry);
                }
            }
        });

        std::sort(out_entries.begin(), out_entries.end(), [](const Entry &entry_a, const Entry &entry_b) {
            return entry_a.index < entry_b.index;
        });
    }

}