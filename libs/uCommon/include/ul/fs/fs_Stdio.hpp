
#pragma once
#include <switch.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <string>

namespace ul::fs {

    template<mode_t Mode>
    inline bool ExistsBase(const std::string &path) {
        struct stat st;
        if(stat(path.c_str(), &st) == 0) {
            return st.st_mode & Mode;
        }
        else {
            return false;
        }
    }

    inline bool ExistsFile(const std::string &path) {
        return ExistsBase<S_IFREG>(path);
    }

    inline bool ExistsDirectory(const std::string &path) {
        return ExistsBase<S_IFDIR>(path);
    }

    inline void CreateDirectory(const std::string &path) {
        mkdir(path.c_str(), 777);
    }

    inline void CreateFile(const std::string &path) {
        fsdevCreateFile(path.c_str(), 0, 0);
    }

    inline void DeleteDirectory(const std::string &path) {
        fsdevDeleteDirectoryRecursively(path.c_str());
    }

    inline void DeleteFile(const std::string &path) {
        remove(path.c_str());
    }

    inline void CleanDirectory(const std::string &path) {
        DeleteDirectory(path);
        CreateDirectory(path);
    }

    inline bool RenameFile(const std::string &old_path, const std::string &new_path) {
        return rename(old_path.c_str(), new_path.c_str()) == 0;
    }

    inline bool WriteFile(const std::string &path, const void *data, const size_t size, const bool overwrite) {
        auto f = fopen(path.c_str(), overwrite ? "wb" : "ab+");
        if(f) {
            fwrite(data, size, 1, f);
            fclose(f);
            return true;
        }
        else {
            return false;
        }
    }

    inline bool ReadFile(const std::string &path, void *data, const size_t size) {
        auto f = fopen(path.c_str(), "rb");
        if(f) {
            const auto ok = fread(data, size, 1, f) == 1;
            fclose(f);
            return ok;
        }
        else {
            return false;
        }
    }

    inline bool ReadFileAtOffset(const std::string &path, const size_t offset, void *data, const size_t size) {
        auto f = fopen(path.c_str(), "rb");
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

    inline size_t GetFileSize(const std::string &path) {
        struct stat st;
        if(stat(path.c_str(), &st) == 0) {
            return st.st_size;
        }
        else {
            return 0;
        }
    }

    inline bool ReadFileString(const std::string &path, std::string &out) {
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

    inline bool WriteFileString(const std::string &path, const std::string &str, const bool overwrite) {
        return WriteFile(path, str.c_str(), str.length(), overwrite);
    }

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