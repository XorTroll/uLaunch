#include <os/os_Titles.hpp>

namespace os {

    std::vector<cfg::TitleRecord> QueryInstalledTitles() {
        std::vector<cfg::TitleRecord> titles;
        UL_OS_FOR_EACH_APP_RECORD(record, {
            cfg::TitleRecord rec = {};
            rec.app_id = record.application_id;
            if(rec.app_id == 0) {
                continue;
            }
            rec.title_type = static_cast<u32>(cfg::TitleType::Installed);
            titles.push_back(rec);
        });
        return titles;
    }

}