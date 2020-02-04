#include <fs/fs_Stdio.hpp>
#include <util/util_String.hpp>

namespace fs
{
    static bool ExistsImpl(size_t st_mode, const std::string &path)
    {
        struct stat st;
        return (stat(path.c_str(), &st) == 0) && (st.st_mode & st_mode);
    }

    bool ExistsFile(const std::string &path)
    {
        return ExistsImpl(S_IFREG, path);
    }

    bool ExistsDirectory(const std::string &path)
    {
        return ExistsImpl(S_IFDIR, path);
    }

    void CreateDirectory(const std::string &path)
    {
        mkdir(path.c_str(), 777);
    }

    void CreateFile(const std::string &path)
    {
        fsdevCreateFile(path.c_str(), 0, 0);
    }

    void CreateConcatenationFile(const std::string &path)
    {
        fsdevCreateFile(path.c_str(), 0, FsCreateOption_BigFile);
    }

    void DeleteDirectory(const std::string &path)
    {
        fsdevDeleteDirectoryRecursively(path.c_str());
    }

    void DeleteFile(const std::string &path)
    {
        remove(path.c_str());
    }

    void MoveFile(const std::string &p1, const std::string &p2)
    {
        rename(p1.c_str(), p2.c_str());
    }

    void CopyFile(const std::string &p, const std::string &np)
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
            }
            fclose(inf);
        }
    }

    static void HandleDirectoryImpl(bool copy, const std::string &d, const std::string &nd)
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

    void MoveDirectory(const std::string &d, const std::string &nd)
    {
        return HandleDirectoryImpl(false, d, nd);
    }

    void CopyDirectory(const std::string &d, const std::string &nd)
    {
        return HandleDirectoryImpl(true, d, nd);
    }
}