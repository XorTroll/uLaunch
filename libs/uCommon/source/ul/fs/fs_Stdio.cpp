#include <ul/fs/fs_Stdio.hpp>
#include <vector>

namespace ul::fs {

    namespace {

        constexpr size_t CopyBufferSize = 0x100000;

    }

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

    void CopyFile(const std::string &src_path, const std::string &dst_path) {
        auto src_f = fopen(src_path.c_str(), "rb");
        if(src_f) {
            auto dst_f = fopen(dst_path.c_str(), "wb");
            if(dst_f) {
                auto copy_buf = new u8[CopyBufferSize]();
                auto rem_size = fs::GetFileSize(src_path);
                while(rem_size > 0) {
                    const auto read_size = fread(copy_buf, 1, std::min(CopyBufferSize, rem_size), src_f);
                    rem_size -= read_size;
                    fwrite(copy_buf, 1, read_size, dst_f);
                }
                delete[] copy_buf;
                fclose(dst_f);
            }
            fclose(src_f);
        }
    }
    
    void CopyDirectory(const std::string &src_path, const std::string &dst_path) {
        fs::CreateDirectory(dst_path);

        UL_FS_FOR(src_path, name, path, is_dir, is_file, {
            const auto src_item_path = fs::JoinPath(src_path, name);
            const auto dst_item_path = fs::JoinPath(dst_path, name);
            if(is_dir) {
                CopyDirectory(src_item_path, dst_item_path);
            }
            else if(is_file) {
                CopyFile(src_item_path, dst_item_path);
            }
        });
    }

}
