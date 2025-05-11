
#pragma once
#include <ul/loader/loader_TargetTypes.hpp>
#include <ul/os/os_Applications.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/ul_Include.hpp>
#include <vector>

namespace ul::menu {

    inline std::string MakeMenuPath(const bool is_emummc, const AccountUid &user) {
        const auto dir = (is_emummc ? "emu" : "sys") + ("_" + util::FormatAccount(user));
        return fs::JoinPath(RootPath, dir);
    }

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
        SpecialEntryAlbum,
        SpecialEntryAmiibo
    };

    struct EntryApplicationInfo {
        u64 app_id;
        NsExtApplicationRecord record;
        NsExtApplicationView view;
        u32 version;
        u32 launch_required_version;

        inline bool HasContents() const {
            return os::ApplicationViewHasFlag<NsExtApplicationViewFlag_HasMainContents>(this->view);
        }

        inline bool IsGameCardInserted() const {
            return os::ApplicationViewHasFlag<NsExtApplicationViewFlag_IsGameCard>(this->view) && os::ApplicationViewHasFlag<NsExtApplicationViewFlag_IsGameCardInserted>(this->view);
        }

        inline bool IsGameCardNotInserted() const {
            return os::ApplicationViewHasFlag<NsExtApplicationViewFlag_IsGameCard>(this->view) && !os::ApplicationViewHasFlag<NsExtApplicationViewFlag_IsGameCardInserted>(this->view);
        }

        inline bool NeedsVerify() const {
            return os::ApplicationViewHasFlag<NsExtApplicationViewFlag_NeedsVerify>(this->view);
        }

        inline bool CanBeLaunched() const {
            return os::ApplicationViewHasFlag<NsExtApplicationViewFlag_CanLaunch>(this->view);
        }

        inline bool IsNotUpdated() const {
            // TODO: is this the right check?
            return R_FAILED(nsCheckApplicationLaunchVersion(this->app_id));
        }
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
                || this->Is<EntryType::SpecialEntryAlbum>()
                || this->Is<EntryType::SpecialEntryAmiibo>();
        }

        inline bool operator<(const Entry &other) const {
            return this->index < other.index;
        }

        inline std::string GetFolderPath() const {
            return fs::JoinPath(fs::GetBaseDirectory(this->entry_path), this->folder_info.fs_name);
        }

        void TryLoadControlData();
        void ReloadApplicationInfo(const bool force_reload_records_views = true);

        void MoveTo(const std::string &new_folder_path);
        bool MoveToIndex(const u32 new_index);
        void OrderSwap(Entry &other_entry);
        
        inline void MoveToParentFolder() {
            const auto parent_path = fs::GetBaseDirectory(fs::GetBaseDirectory(this->entry_path));
            this->MoveTo(parent_path);
        }
        
        inline void MoveToRoot(const std::string &menu_path) {
            this->MoveTo(menu_path);
        }

        void Save() const;
        std::vector<Entry> Remove();
    };

    void SetLoadApplicationEntryVersions(const bool load);

    void InitializeEntries(const bool is_emummc, const AccountUid &uid);
    std::vector<Entry> LoadEntries(const std::string &path);
    const std::string &GetActiveMenuPath();
    
    void EnsureApplicationEntry(const NsExtApplicationRecord &app_record, const std::string &menu_path = "");
    Entry CreateFolderEntry(const std::string &base_path, const std::string &folder_name, const s32 index = -1);
    Entry CreateHomebrewEntry(const std::string &base_path, const std::string &nro_path, const std::string &nro_argv, const s32 index = -1);
    Entry CreateSpecialEntry(const std::string &base_path, const EntryType type, const s32 index = -1);
    void DeleteApplicationEntryRecursively(const u64 app_id, const std::string &path);

    void ReloadApplicationEntryInfos(std::vector<Entry> &entries);

}
