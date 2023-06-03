#include <ul/menu/thm/thm_Theme.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <extras/json.hpp>

namespace ul::menu::thm {

    std::string Theme::LoadMenuSource(const MenuId id) {
        std::string src;

        std::string id_str;
        if(ConvertMenuIdToString(id, id_str)) {
            const auto src_path = this->path + "/src/" + id_str + ".js";
            fs::ReadFileString(src_path, src);
        }

        return src;
    }

    std::vector<Theme> LoadThemes() {
        std::vector<Theme> themes;
        UL_FS_FOR(ThemeDirectory, entry_name, entry_path, is_dir, is_file, {
            if(is_dir) {
                const auto manifest_path = entry_path + "/theme/Manifest.json";
                if(fs::ExistsFile(manifest_path)) {
                    std::string manifest_json_str;
                    if(fs::ReadFileString(manifest_path, manifest_json_str)) {
                        try {
                            const auto manifest_json = nlohmann::json::parse(manifest_json_str);

                            const auto name = manifest_json.value("name", "");
                            const auto format_version = manifest_json.value("format_version", 0u);
                            const auto release = manifest_json.value("release", "");
                            const auto description = manifest_json.value("description", "");
                            const auto author = manifest_json.value("author", "");
                            if(!name.empty() && (format_version == 1)) {
                                const Theme theme = {
                                    .manifest = {
                                        .name = name,
                                        .format_version = format_version,
                                        .release = release,
                                        .description = description,
                                        .author = author
                                    },
                                    .path = entry_path
                                };
                                themes.push_back(theme);
                            }
                        }
                        catch(std::exception&) {}
                    }
                }
            }
        });

        return themes;
    }

}