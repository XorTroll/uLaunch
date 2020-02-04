
#pragma once
#include <ul_Include.hpp>

namespace fs
{
    bool ExistsFile(const std::string &path);
    bool ExistsDirectory(const std::string &path);

    void CreateDirectory(const std::string &path);
    void CreateFile(const std::string &path);
    void CreateConcatenationFile(const std::string &path);

    void DeleteDirectory(const std::string &path);
    void DeleteFile(const std::string &path);

    inline bool WriteFile(const std::string &path, void *data, size_t size, bool overwrite)
    {
        FILE *f = fopen(path.c_str(), overwrite ? "wb" : "ab+");
        if(f)
        {
            fwrite(data, 1, size, f);
            fclose(f);
            return true;
        }
        return false;
    }

    inline bool ReadFile(const std::string &path, void *data, size_t size)
    {
        FILE *f = fopen(path.c_str(), "rb");
        if(f)
        {
            fread(data, 1, size, f);
            fclose(f);
            return true;
        }
        return false;
    }

    inline size_t GetFileSize(const std::string &path)
    {
        FILE *f = fopen(path.c_str(), "rb");
        if(f)
        {
            fseek(f, 0, SEEK_END);
            size_t fsz = ftell(f);
            rewind(f);
            fclose(f);
            return fsz;
        }
        return 0;
    }

    void MoveFile(const std::string &p1, const std::string &p2);
    void CopyFile(const std::string &p, const std::string &np);

    void MoveDirectory(const std::string &d, const std::string &nd);
    void CopyDirectory(const std::string &d, const std::string &nd);

    #define FS_FOR(dir, name_var, path_var, ...) \
        DIR *dp = opendir(dir.c_str()); \
        if(dp) \
        { \
            dirent *dt; \
            while(true) \
            { \
                dt = readdir(dp); \
                if(dt == NULL) break; \
                std::string name_var = dt->d_name; \
                std::string path_var = dir + "/" + dt->d_name; \
                __VA_ARGS__ \
            } \
            closedir(dp); \
        }
}