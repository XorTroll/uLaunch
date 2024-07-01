#include <ul/fs/fs_Stdio.hpp>
#include <vector>

namespace ul::fs {

    void RenameDirectory(const std::string &old_path, const std::string &new_path) {
        fs::CreateDirectory(new_path);
        std::vector<std::pair<std::string, std::string>> rename_table;
        UL_FS_FOR(old_path, entry_name, entry_path, is_dir, is_file, {
            const auto new_entry_path = fs::JoinPath(new_path, entry_name);
            if(is_dir) {
                RenameDirectory(entry_path, new_entry_path);
            }
            else if(is_file) {
                rename_table.push_back(std::make_pair(entry_path, new_entry_path));
            }
        });
        for(const auto &[src, dst] : rename_table) {
            fs::RenameFile(src, dst);
        }

        fs::DeleteDirectory(old_path);
    }

}
