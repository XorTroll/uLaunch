
#pragma once
#include <stratosphere.hpp>

namespace ecs
{
    // Slightly modified version of ams's RemoteFileSystem
    
    class RemoteFileSystem : public ams::fs::fsa::IFileSystem {
        private:
            FsFileSystem *base_fs;
            bool close_fs;
        public:
            RemoteFileSystem(::FsFileSystem *fs, bool close) : base_fs(fs), close_fs(close) { /* ... */ }

            virtual ~RemoteFileSystem() {
                if(close_fs) {
                    fsFsClose(this->base_fs);
                }
            }

        public:
            virtual ams::Result CreateFileImpl(const char *path, s64 size, int flags) override final {
                return fsFsCreateFile(this->base_fs, path, size, flags);
            }

            virtual ams::Result DeleteFileImpl(const char *path) override final {
                return fsFsDeleteFile(this->base_fs, path);
            }

            virtual ams::Result CreateDirectoryImpl(const char *path) override final {
                return fsFsCreateDirectory(this->base_fs, path);
            }

            virtual ams::Result DeleteDirectoryImpl(const char *path) override final {
                return fsFsDeleteDirectory(this->base_fs, path);
            }

            virtual ams::Result DeleteDirectoryRecursivelyImpl(const char *path) override final {
                return fsFsDeleteDirectoryRecursively(this->base_fs, path);
            }

            virtual ams::Result RenameFileImpl(const char *old_path, const char *new_path) override final {
                return fsFsRenameFile(this->base_fs, old_path, new_path);
            }

            virtual ams::Result RenameDirectoryImpl(const char *old_path, const char *new_path) override final {
                return fsFsRenameDirectory(this->base_fs, old_path, new_path);
            }

            virtual ams::Result GetEntryTypeImpl(ams::fs::DirectoryEntryType *out, const char *path) override final {
                static_assert(sizeof(::FsDirEntryType) == sizeof(ams::fs::DirectoryEntryType));
                return fsFsGetEntryType(this->base_fs, path, reinterpret_cast<::FsDirEntryType *>(out));
            }

            virtual ams::Result OpenFileImpl(std::unique_ptr<ams::fs::fsa::IFile> *out_file, const char *path, ams::fs::OpenMode mode) override final {
                FsFile f;
                R_TRY(fsFsOpenFile(this->base_fs, path, mode, &f));

                *out_file = std::make_unique<ams::fs::RemoteFile>(f);
                return ams::ResultSuccess();
            }

            virtual ams::Result OpenDirectoryImpl(std::unique_ptr<ams::fs::fsa::IDirectory> *out_dir, const char *path, ams::fs::OpenDirectoryMode mode) override final {
                FsDir d;
                R_TRY(fsFsOpenDirectory(this->base_fs, path, mode, &d));

                *out_dir = std::make_unique<ams::fs::RemoteDirectory>(d);
                return ams::ResultSuccess();
            }

            virtual ams::Result CommitImpl() override final {
                return fsFsCommit(this->base_fs);
            }


            virtual ams::Result GetFreeSpaceSizeImpl(s64 *out, const char *path) {
                return fsFsGetFreeSpace(this->base_fs, path, out);
            }

            virtual ams::Result GetTotalSpaceSizeImpl(s64 *out, const char *path) {
                return fsFsGetTotalSpace(this->base_fs, path, out);
            }

            virtual ams::Result CleanDirectoryRecursivelyImpl(const char *path) {
                return fsFsCleanDirectoryRecursively(this->base_fs, path);
            }

            virtual ams::Result GetFileTimeStampRawImpl(ams::fs::FileTimeStampRaw *out, const char *path) {
                static_assert(sizeof(ams::fs::FileTimeStampRaw) == sizeof(::FsTimeStampRaw));
                return fsFsGetFileTimeStampRaw(this->base_fs, path, reinterpret_cast<::FsTimeStampRaw *>(out));
            }

            virtual ams::Result QueryEntryImpl(char *dst, size_t dst_size, const char *src, size_t src_size, ams::fs::fsa::QueryId query, const char *path) {
                return fsFsQueryEntry(this->base_fs, dst, dst_size, src, src_size, path, static_cast<FsFileSystemQueryId>(query));
            }
    };
}