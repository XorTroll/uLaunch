
#pragma once
#include <q_Include.hpp>

namespace fs
{
    bool ExistsFile(std::string path);
    bool ExistsDirectory(std::string path);

    void CreateDirectory(std::string path);
    void CreateFile(std::string path);
    void CreateConcatenationFile(std::string path);

    void DeleteDirectory(std::string path);
    void DeleteFile(std::string path);

    inline bool WriteFile(std::string path, void *data, size_t size, bool overwrite)
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

    inline bool ReadFile(std::string path, void *data, size_t size)
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

    inline size_t GetFileSize(std::string path)
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

    void MoveFile(std::string p1, std::string p2);
    void CopyFile(std::string p, std::string np);

    void MoveDirectory(std::string d, std::string nd);
    void CopyDirectory(std::string d, std::string nd);

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