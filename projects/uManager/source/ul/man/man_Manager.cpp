#include <ul/man/man_Manager.hpp>
#include <ul/ul_Include.hpp>
#include <ul/fs/fs_Stdio.hpp>

namespace ul::man {

    namespace {

        inline bool ExistsDirectory(const char *path) {
            struct stat st;
            if(stat(path, &st) == 0) {
                return st.st_mode & S_IFDIR;
            }
            return false;
        }

        constexpr size_t CopyBufferSize = 0x100000;

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

    bool IsBasePresent() {
        return ExistsDirectory(BaseSystemPath);
    }

    bool IsSystemActive() {
        return ExistsDirectory(ActiveSystemPath);
    }

    void ActivateSystem() {
        CopyDirectory(BaseSystemPath, ActiveSystemPath);
    }

    void DeactivateSystem() {
        fsdevDeleteDirectoryRecursively(ActiveSystemPath);
    }

    std::string Version::AsString() const {
        auto as_str = std::to_string(this->major) + "." + std::to_string(this->minor);
        if(this->micro > 0) {
            as_str += "." + std::to_string(this->micro);
        }
        return as_str;
    }

    Version Version::FromString(const std::string &ver_str) {
        auto ver_str_cpy = ver_str;
        Version v = {};
        size_t pos = 0;
        std::string token;
        u32 c = 0;
        std::string delimiter = ".";
        while((pos = ver_str_cpy.find(delimiter)) != std::string::npos) {
            token = ver_str_cpy.substr(0, pos);
            if(c == 0) {
                v.major = std::stoi(token);
            }
            else if(c == 1) {
                v.minor = std::stoi(token);
            }
            else if(c == 2) {
                v.micro = std::stoi(token);
            }
            ver_str_cpy.erase(0, pos + delimiter.length());
            c++;
        }

        if(c == 0) {
            v.major = std::stoi(ver_str_cpy);
        }
        else if(c == 1) {
            v.minor = std::stoi(ver_str_cpy);
        }
        else if(c == 2) {
            v.micro = std::stoi(ver_str_cpy);
        }
        return v;
    }

}