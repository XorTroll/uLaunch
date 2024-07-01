
#pragma once
#include <ul/loader/loader_TargetTypes.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/ul_Include.hpp>
#include <vector>

namespace ul::menu {

    enum class EntryType : u32 {
        Invalid,
        Application,
        Homebrew,
        Folder,
        SpecialEntryMiiEdit,
        SpecialEntryWebBrowser,
        SpecialEntryUserPage,
        SpecialEntrySettings,
        SpecialEntryThemes,
        SpecialEntryControllers,
        SpecialEntryAlbum
    };

    struct EntryApplicationInfo {
        u64 app_id;
        NsApplicationRecord record;
        NsApplicationContentMetaStatus meta_status;

        inline bool IsInstalledNew() const {
            return this->record.type == 0x03;
        }

        inline bool IsInstalled() const {
            return this->record.type == 0x10;
        }

        inline bool IsRunning() const {
            return this->record.type == 0x0; // Not really an ideal state, might be after a uLaunch crash, but whatever
        }
        
        inline bool IsLaunchable() const {
            return this->IsInstalled() || this->IsInstalledNew() || this->IsRunning();
        }

        /* TODO (new)
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

        inline constexpr bool IsSpecial() const {
            return this->Is<EntryType::SpecialEntryMiiEdit>()
                || this->Is<EntryType::SpecialEntryWebBrowser>()
                || this->Is<EntryType::SpecialEntryUserPage>()
                || this->Is<EntryType::SpecialEntrySettings>()
                || this->Is<EntryType::SpecialEntryThemes>()
                || this->Is<EntryType::SpecialEntryControllers>()
                || this->Is<EntryType::SpecialEntryAlbum>();
        }

        inline bool operator<(const Entry &other) const {
            return this->index < other.index;
        }

        inline std::string GetFolderPath() const {
            return fs::JoinPath(fs::GetBaseDirectory(this->entry_path), this->folder_info.fs_name);
        }

        void TryLoadControlData();
        void ReloadApplicationInfo();

        void MoveTo(const std::string &new_folder_path);
        bool MoveToIndex(const u32 new_index);
        void OrderSwap(Entry &other_entry);
        
        inline void MoveToParentFolder() {
            const auto parent_path = fs::GetBaseDirectory(fs::GetBaseDirectory(this->entry_path));
            this->MoveTo(parent_path);
        }
        
        inline void MoveToRoot() {
            this->MoveTo(MenuPath);
        }

        void Save() const;
        std::vector<Entry> Remove();
    };

    void InitializeEntries();
    void EnsureApplicationEntry(const NsApplicationRecord &app_record);

    std::vector<Entry> LoadEntries(const std::string &path);
    
    Entry CreateFolderEntry(const std::string &base_path, const std::string &folder_name, const u32 index);
    Entry CreateHomebrewEntry(const std::string &base_path, const std::string &nro_path, const std::string &nro_argv, const u32 index);
    void DeleteApplicationEntry(const u64 app_id, const std::string &path);

}
