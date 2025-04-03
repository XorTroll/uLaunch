
#pragma once
#include <switch.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ul/util/util_String.hpp>

namespace ul::fs {

    template<typename S, mode_t Mode>
    inline bool ExistsBase(const S &path) {
        struct stat st;
        if(stat(util::GetCString(path), &st) == 0) {
            return st.st_mode & Mode;
        }
        else {
            return false;
        }
    }

    template<typename S>
    inline bool ExistsFile(const S &path) {
        return ExistsBase<S, S_IFREG>(path);
    }

    template<typename S>
    inline bool ExistsDirectory(const S &path) {
        return ExistsBase<S, S_IFDIR>(path);
    }

    template<typename S>
    inline void CreateDirectory(const S &path) {
        mkdir(util::GetCString(path), 777);
    }

    template<typename S>
    inline void CreateFile(const S &path) {
        fsdevCreateFile(util::GetCString(path), 0, 0);
    }

    template<typename S>
    inline void DeleteDirectory(const S &path) {
        fsdevDeleteDirectoryRecursively(util::GetCString(path));
    }

    template<typename S>
    inline void DeleteFile(const S &path) {
        remove(util::GetCString(path));
    }

    template<typename S>
    inline void CleanDirectory(const S &path) {
        DeleteDirectory(path);
        CreateDirectory(path);
    }

    template<typename S>
    inline bool RenameFile(const S &old_path, const S &new_path) {
        return rename(util::GetCString(old_path), util::GetCString(new_path)) == 0;
    }

    template<typename S>
    inline bool WriteFile(const S &path, const void *data, const size_t size, const bool overwrite) {
        auto f = fopen(util::GetCString(path), overwrite ? "wb" : "ab+");
        if(f) {
            fwrite(data, size, 1, f);
            fclose(f);
            return true;
        }
        else {
            return false;
        }
    }

    template<typename S>
    inline bool ReadFile(const S &path, void *data, const size_t size) {
        auto f = fopen(util::GetCString(path), "rb");
        if(f) {
            const auto ok = fread(data, size, 1, f) == 1;
            fclose(f);
            return ok;
        }
        else {
            return false;
        }
    }

    template<typename S>
    inline bool ReadFileAtOffset(const S &path, const size_t offset, void *data, const size_t size) {
        auto f = fopen(util::GetCString(path), "rb");
        if(f) {
            fseek(f, offset, SEEK_SET);
            const auto ok = fread(data, size, 1, f) == 1;
            fclose(f);
            return ok;
        }
        else {
            return false;
        }
    }

    template<typename S>
    inline size_t GetFileSize(const S &path) {
        struct stat st;
        if(stat(util::GetCString(path), &st) == 0) {
            return st.st_size;
        }
        else {
            return 0;
        }
    }

    template<typename S>
    inline bool ReadFileString(const S &path, std::string &out) {
        const auto f_size = GetFileSize(path);
        if(f_size == 0) {
            return false;
        }

        auto str_buf = new char[f_size + 1]();
        const auto ok = ReadFile(path, str_buf, f_size);
        if(ok) {
            out.assign(str_buf, f_size);
        }
        delete[] str_buf;
        return ok;
    }

    template<typename S>
    inline bool WriteFileString(const S &path, const std::string &str, const bool overwrite) {
        return WriteFile(path, str.c_str(), str.length(), overwrite);
    }

    inline std::string GetBaseName(const std::string &path) {
        return path.substr(path.find_last_of("/") + 1);
    }

    inline std::string GetBaseDirectory(const std::string &path) {
        return path.substr(0, path.find_last_of("/"));
    }

    inline std::string GetFileName(const std::string &path) {
        return path.substr(0, path.find_last_of("."));
    }

    inline std::string GetExtension(const std::string &path) {
        return path.substr(path.find_last_of(".") + 1);
    }

    inline std::string JoinPath(const std::string &a, const std::string &b) {
        return a + "/" + b;
    }

    template<typename S>
    inline void EnsureCreateDirectory(const S &path) {
        const auto parent_path = GetBaseDirectory(path);
        if(parent_path.back() != ':') {
            EnsureCreateDirectory(parent_path);
        }
        mkdir(util::GetCString(path), 777);
    }

    void RenameDirectory(const std::string &old_path, const std::string &new_path);

    void CopyFile(const std::string &src_path, const std::string &dst_path);
    void CopyDirectory(const std::string &src_path, const std::string &dst_path);

    #define UL_FS_FOR(dir, name_var, path_var, is_dir_var, is_file_var, ...) ({ \
        const std::string dir_str = (dir); \
        auto dp = opendir(dir_str.c_str()); \
        if(dp) { \
            while(true) { \
                auto dt = readdir(dp); \
                if(dt == nullptr) { \
                    break; \
                } \
                const std::string name_var = dt->d_name; \
                (void)(name_var); \
                const std::string path_var = dir_str + "/" + dt->d_name; \
                (void)(path_var); \
                const auto is_dir_var = dt->d_type & DT_DIR; \
                (void)(is_dir_var); \
                const auto is_file_var = dt->d_type & DT_REG; \
                (void)(is_file_var); \
                __VA_ARGS__ \
            } \
            closedir(dp); \
        } \
    })

}
