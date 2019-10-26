#include <fs/fs_Stdio.hpp>
#include <util/util_String.hpp>
#include <db/db_Save.hpp>

namespace fs
{
    static bool ExistsImpl(size_t st_mode, std::string path)
    {
        struct stat st;
        return (stat(path.c_str(), &st) == 0) && (st.st_mode & st_mode);
    }

    bool ExistsFile(std::string path)
    {
        return ExistsImpl(S_IFREG, path);
    }

    bool ExistsDirectory(std::string path)
    {
        return ExistsImpl(S_IFDIR, path);
    }

    void CreateDirectory(std::string path)
    {
        mkdir(path.c_str(), 777);
        db::Commit();
    }

    void CreateFile(std::string path)
    {
        fsdevCreateFile(path.c_str(), 0, 0);
        db::Commit();
    }

    void CreateConcatenationFile(std::string path)
    {
        fsdevCreateFile(path.c_str(), 0, FS_CREATE_BIG_FILE);
        db::Commit();
    }

    void DeleteDirectory(std::string path)
    {
        fsdevDeleteDirectoryRecursively(path.c_str());
        db::Commit();
    }

    void DeleteFile(std::string path)
    {
        remove(path.c_str());
        db::Commit();
    }

    bool WriteFile(std::string path, void *data, size_t size, bool overwrite)
    {
        FILE *f = fopen(path.c_str(), overwrite ? "wb" : "ab+");
        if(f)
        {
            fwrite(data, 1, size, f);
            fclose(f);
            db::Commit();
            return true;
        }
        return false;
    }

    bool ReadFile(std::string path, void *data, size_t size)
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

    size_t GetFileSize(std::string path)
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

    static void ForEachInImpl(size_t dt_type, std::string dir, std::function<void(std::string name, std::string path)> fn)
    {
        DIR *dp = opendir(dir.c_str());
        if(dp)
        {
            dirent *dt;
            while(true)
            {
                dt = readdir(dp);
                if(dt == NULL) break;
                if(dt->d_type & dt_type) fn(dt->d_name, dir + "/" + dt->d_name);
            }
            closedir(dp);
        }
    }

    void ForEachFileIn(std::string dir, std::function<void(std::string name, std::string path)> fn)
    {
        return ForEachInImpl(DT_REG, dir, fn);
    }

    void ForEachDirectoryIn(std::string dir, std::function<void(std::string name, std::string path)> fn)
    {
        return ForEachInImpl(DT_DIR, dir, fn);
    }

    void MoveFile(std::string p1, std::string p2)
    {
        rename(p1.c_str(), p2.c_str());
        db::Commit();
        db::Commit();
    }

    void CopyFile(std::string p, std::string np)
    {
        FILE *inf = fopen(p.c_str(), "rb");
        if(inf)
        {
            FILE *outf = fopen(np.c_str(), "wb");
            if(outf)
            {
                fseek(inf, 0, SEEK_END);
                u64 fsize = ftell(inf);
                rewind(inf);
                u64 readsize = 0x4000;
                u64 tocopy = fsize;
                u8 *tmp = new u8[readsize]();
                memset(tmp, 0, readsize);
                while(tocopy)
                {
                    auto read = fread(tmp, 1, std::min(readsize, tocopy), inf);
                    fwrite(tmp, 1, read, outf);
                    tocopy -= read;
                }
                delete[] tmp;
                fclose(outf);
                db::Commit();
            }
            fclose(inf);
        }
    }

    static void HandleDirectoryImpl(bool copy, std::string d, std::string nd)
    {
        DIR *dp = opendir(d.c_str());
        CreateDirectory(nd);
        if(dp)
        {
            dirent *dt;
            while(true)
            {
                dt = readdir(dp);
                if(dt == NULL) break;
                std::string fullp = d + "/" + std::string(dt->d_name);
                std::string nfullp = nd + "/" + std::string(dt->d_name);
                if(dt->d_type & DT_DIR) HandleDirectoryImpl(copy, fullp, nfullp);
                else if(dt->d_type & DT_REG)
                {
                    if(copy) CopyFile(fullp, nfullp);
                    else MoveFile(fullp, nfullp);
                }
            }
            closedir(dp);
        }
        if(!copy) DeleteDirectory(d);
    }

    void MoveDirectory(std::string d, std::string nd)
    {
        return HandleDirectoryImpl(false, d, nd);
    }

    void CopyDirectory(std::string d, std::string nd)
    {
        return HandleDirectoryImpl(true, d, nd);
    }
}