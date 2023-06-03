
#pragma once
#include <ul/util/util_String.hpp>
#include <vector>

namespace ul::ent {

    enum class EntryKind : u32 {
        Invalid,
        Folder,
        Application,
        Homebrew
    };

    inline bool TryParseEntryKind(const std::string &raw_kind, EntryKind &out_kind) {
        if(raw_kind == "application") {
            out_kind = EntryKind::Application;
            return true;
        }
        if(raw_kind == "homebrew") {
            out_kind = EntryKind::Homebrew;
            return true;
        }
        if(raw_kind == "folder") {
            out_kind = EntryKind::Folder;
            return true;
        }

        return false;
    }

    struct EntryApplicationMetadata {
        u64 app_id;
    };

    struct EntryHomebrewMetadata {
        std::string nro_path;
        std::string nro_argv;
    };

    struct EntryMetadata {
        std::string name;
        std::string author;
        std::string version;
        std::string icon_path;
        EntryApplicationMetadata app_metadata;
        EntryHomebrewMetadata hb_metadata;

        inline bool SetFromNacp(const NacpStruct &nacp) {
            NacpLanguageEntry *lang_entry;
            nacpGetLanguageEntry(const_cast<NacpStruct*>(std::addressof(nacp)), &lang_entry);
            if(lang_entry == nullptr) {
                return false;
            }

            this->name = lang_entry->name;
            this->author = lang_entry->author;
            this->version = nacp.display_version;
            return true;
        }
    };

    struct EntryPath {
        std::vector<std::string> items;

        inline bool IsRoot() const {
            return this->items.empty();
        }

        inline bool operator==(const EntryPath &other) const {
            if(this->items.size() != other.items.size()) {
                return false;
            }

            for(u32 i = 0; i < this->items.size(); i++) {
                if(this->items.at(i) != other.items.at(i)) {
                    return false;
                }
            }

            return true;
        }

        inline bool operator!=(const EntryPath &other) const {
            return !this->operator==(other);
        }

        inline EntryPath operator+(const std::string &dir) const {
            auto new_path = *this;
            new_path.Push(dir);
            return new_path;
        }

        inline void Push(const std::string &dir) {
            this->items.push_back(dir);
        }

        inline void Pop() {
            this->items.pop_back();
        }
    };

    struct Entry {
        EntryKind kind;
        EntryPath path;
        std::string name;
        u32 index;
        EntryMetadata metadata;

        Entry() : kind(EntryKind::Invalid) {}

        template<EntryKind Kind>
        inline constexpr bool Is() const {
            return this->kind == Kind;
        }

        inline constexpr bool IsValid() const {
            return !this->Is<EntryKind::Invalid>();
        }

        inline constexpr bool IsNormalEntry() const {
            return this->Is<EntryKind::Application>() || this->Is<EntryKind::Homebrew>();
        }

        inline bool operator==(const Entry &other) {
            if(this->kind == other.kind) {
                if(this->path == other.path) {
                    if(this->Is<EntryKind::Application>()) {
                        return this->metadata.app_metadata.app_id == other.metadata.app_metadata.app_id;
                    }
                    else if(this->Is<EntryKind::Homebrew>()) {
                        return (this->metadata.hb_metadata.nro_path == other.metadata.hb_metadata.nro_path) && (this->metadata.hb_metadata.nro_argv == other.metadata.hb_metadata.nro_argv);
                    }
                    else if(this->Is<EntryKind::Folder>()) {
                        return this->metadata.name == other.metadata.name;
                    }
                }
            }
            return false;
        }

        bool UpdatePath(const EntryPath &new_path);
        bool Move(const u32 new_idx);

        inline bool Remove() {
            return this->Move(UINT32_MAX);
        }
    };

    std::string GetHomebrewIdentifier(const std::string &nro_path);

    void EnsureCreatePath(const EntryPath &path);

    constexpr const char RootDirectory[] = "sdmc:/umad/entries";
    constexpr const char CacheDirectory[] = "sdmc:/umad/entry_cache";
    constexpr const char DefaultHomebrewIconPath[] = "sdmc:/umad/default_hb_icon.jpg";
    constexpr const char DefaultHomebrewNacpPath[] = "sdmc:/umad/default_hb_nacp.nacp";

    constexpr const char FolderMetaEntryMame[] = "folder-meta.json";

    inline std::string GetCacheIcon(const std::string &cache_name) {
        return CacheDirectory + ("/" + cache_name) + ".jpg";
    }

    inline std::string GetCacheNacp(const std::string &cache_name) {
        return CacheDirectory + ("/" + cache_name) + ".nacp";
    }
    
    inline std::string GetApplicationCacheIcon(const u64 app_id) {
        return GetCacheIcon(util::FormatProgramId(app_id));
    }

    inline std::string GetApplicationCacheNacp(const u64 app_id) {
        return GetCacheNacp(util::FormatProgramId(app_id));
    }

    inline std::string GetHomebrewCacheIcon(const std::string &nro_path) {
        return GetCacheIcon(GetHomebrewIdentifier(nro_path));
    }

    inline std::string GetHomebrewCacheNacp(const std::string &nro_path) {
        return GetCacheNacp(GetHomebrewIdentifier(nro_path));
    }

    std::string ConvertEntryPath(const EntryPath &path);

}