
#pragma once
#include <ul/loader/loader_TargetTypes.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <vector>

namespace ul::menu {

    enum class EntryType : u32 {
        Invalid,
        Application,
        Homebrew,
        Folder
    };

    struct EntryApplicationInfo {
        NsApplicationRecord record;
        NsApplicationContentMetaStatus meta_status;

        inline bool IsInstalledNew() const {
            return this->record.type == 0x03;
        }

        inline bool IsInstalled() const {
            return this->record.type == 0x10;
        }
        
        inline bool IsLaunchable() const {
            return this->IsInstalled() || this->IsInstalledNew();
        }

        /* TODONEW
        inline bool IsGamecard() const {
            return this->meta_status.storageID == NcmStorageId_GameCard;
        }
        */
    };

    struct EntryHomebrewInfo {
        loader::TargetInput nro_target;
    };

    struct EntryFolderInfo {
        char name[FS_MAX_PATH];
        char fs_name[FS_MAX_PATH];
    };

    struct EntryControlData {
        std::string name;
        bool custom_name;
        std::string author;
        bool custom_author;
        std::string version;
        bool custom_version;
        std::string icon_path;
        bool custom_icon_path;

        inline bool IsLoaded() {
            return !this->name.empty() && !this->author.empty() && !this->version.empty() && !this->icon_path.empty();
        }
    };

    constexpr u32 InvalidEntryIndex = UINT32_MAX;

    struct Entry {
        EntryType type;
        std::string entry_path;
        u32 index;
        EntryControlData control;

        union {
            EntryApplicationInfo app_info;
            EntryHomebrewInfo hb_info;
            EntryFolderInfo folder_info;
        };

        template<EntryType Type>
        inline constexpr bool Is() const {
            return this->type == Type;
        }

        inline bool operator<(const Entry &other) const {
            return this->index < other.index;
        }

        inline std::string GetFolderPath() const {
            return fs::JoinPath(fs::GetBaseDirectory(this->entry_path), this->folder_info.fs_name);
        }

        void TryLoadControlData();
        void MoveTo(const std::string &new_folder_path);
        void Save() const;
        void Remove();

        void OrderBetween(const u32 start_idx, const u32 end_idx);
        void OrderSwap(Entry &other_entry);
    };

    void InitializeEntries();
    void EnsureApplicationEntry(const NsApplicationRecord &app_record, const u32 start_idx = InvalidEntryIndex, const u32 end_idx = InvalidEntryIndex);

    std::vector<Entry> LoadEntries(const std::string &path);
    
    Entry CreateFolderEntry(const std::string &base_path, const std::string &folder_name);
    Entry CreateHomebrewEntry(const std::string &base_path, const std::string &nro_path, const std::string &nro_argv);

}