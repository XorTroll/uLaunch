
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

    size_t GetFileSize(std::string path);

    void ForEachFileIn(std::string dir, std::function<void(std::string name, std::string path)> fn);
    void ForEachDirectoryIn(std::string dir, std::function<void(std::string name, std::string path)> fn);

    void MoveFile(std::string p1, std::string p2);
    void CopyFile(std::string p, std::string np);

    void MoveDirectory(std::string d, std::string nd);
    void CopyDirectory(std::string d, std::string nd);
}