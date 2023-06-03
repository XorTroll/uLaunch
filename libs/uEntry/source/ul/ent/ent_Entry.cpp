#include <ul/ent/ent_Entry.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <extras/json.hpp>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace ul::ent {

    namespace {

        u32 FindCurrentIndex(const std::string &path) {
            u32 idx = 0;
            while(fs::ExistsFile(path + "/" + std::to_string(idx) + ".json")) {
                idx++;
            }
            return idx;
        }

        void CreateDirectoryEntry(const std::string &base_path, const std::string &folder_name, const u32 cur_idx) {
            const auto name = std::to_string(cur_idx) + ".json";
            const EntryPath path = {};
            const auto entry_json_path = base_path + "/" + name;
            auto entry_json = nlohmann::json::object();
            entry_json["kind"] = "folder";
            entry_json["name"] = folder_name;

            const auto entry_json_str = entry_json.dump(4);
            fs::WriteFileString(entry_json_path, entry_json_str, true);
        }

        inline void EnsureEntryPath(const EntryPath &path) {
            std::string actual_path = RootDirectory;
            for(const auto &item : path.items) {
                const auto actual_base_path = actual_path;
                actual_path += "/";
                actual_path += item;
                if(!fs::ExistsDirectory(actual_path)) {
                    const auto cur_idx = FindCurrentIndex(actual_base_path);
                    CreateDirectoryEntry(actual_base_path, item, cur_idx);
                    fs::CreateDirectory(actual_path);
                }
            }
        }

    }

    bool Entry::UpdatePath(const EntryPath &new_path) {
        EnsureEntryPath(new_path);
        const auto actual_new_path = ConvertEntryPath(new_path);
        const auto new_idx = FindCurrentIndex(actual_new_path);
        const auto old_entry_json_path = ConvertEntryPath(this->path) + "/" + this->name;
        const auto new_entry_json_path = actual_new_path + "/" + std::to_string(new_idx) + ".json";

        if(!fs::RenameFile(old_entry_json_path, new_entry_json_path)) {
            return false;
        }

        this->path = new_path;
        return true;
    }

    bool Entry::Move(const u32 new_idx) {
        if(this->index == new_idx) {
            return true;
        }

        const auto actual_path = ConvertEntryPath(this->path);
        const auto old_path = actual_path + "/" + this->name;

        const auto str_new_idx = std::to_string(new_idx);
        
        const auto temp_name = str_new_idx + "_tmp.json";
        const auto temp_path = actual_path + "/" + temp_name;

        const auto new_name = str_new_idx + ".json";
        const auto new_path = actual_path + "/" + new_name;

        std::vector<std::string> old_paths = { old_path };
        std::vector<std::string> temp_paths = { temp_path };
        std::vector<std::string> new_paths = { new_path };
        UL_FS_FOR(actual_path, entry_name, entry_path, is_dir, is_file, {
            if(is_file && util::StringEndsWith(entry_name, ".json")) {
                const auto raw_idx = entry_name.substr(0, entry_name.length() - 5);
                const auto idx = strtoul(raw_idx.c_str(), nullptr, 10);
                if((this->index < new_idx) && (idx > this->index) && (idx <= new_idx)) {
                    old_paths.push_back(entry_path);
                    const auto str_idx = std::to_string(idx - 1);
                    temp_paths.push_back(actual_path + "/" + str_idx + "_tmp.json");
                    new_paths.push_back(actual_path + "/" + str_idx + ".json");
                }
                else if((this->index > new_idx) && (idx < this->index) && (idx >= new_idx)) {
                    old_paths.push_back(entry_path);
                    const auto str_idx = std::to_string(idx + 1);
                    temp_paths.push_back(actual_path + "/" + str_idx + "_tmp.json");
                    new_paths.push_back(actual_path + "/" + str_idx + ".json");
                }
            }
        });

        for(u32 i = 0; i < old_paths.size(); i++) {
            fs::RenameFile(old_paths.at(i), temp_paths.at(i));
        }

        for(u32 i = 0; i < temp_paths.size(); i++) {
            fs::RenameFile(temp_paths.at(i), new_paths.at(i));
        }

        if(new_idx == UINT32_MAX) {
            // Remove the entry
            fs::DeleteFile(new_path);
        }

        this->index = new_idx;
        this->name = new_name;
        return true;
    }

    void EnsureCreatePath(const EntryPath &path) {
        EnsureEntryPath(path);
    }

    std::string GetHomebrewIdentifier(const std::string &nro_path) {
        char nro_hash_path[FS_MAX_PATH] = {};
        strcpy(nro_hash_path, nro_path.c_str());
        u8 hash[SHA256_HASH_SIZE] = {};
        sha256CalculateHash(hash, nro_hash_path, sizeof(nro_hash_path));

        std::stringstream strm;
        // Use the first half of the hash, like Nintendo does with NCAs
        for(u32 i = 0; i < sizeof(hash) / 2; i++) {
            strm << std::setw(2) << std::setfill('0') << std::hex << std::nouppercase << static_cast<u32>(hash[i]);
        }
        return strm.str();
    }

    std::string ConvertEntryPath(const EntryPath &path) {
        std::string actual_path = "sdmc:/umad/entries";
        for(const auto &item : path.items) {
            actual_path += "/";
            actual_path += item;
        }
        return actual_path;
    }

}