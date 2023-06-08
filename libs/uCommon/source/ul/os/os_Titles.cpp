#include <ul/os/os_Titles.hpp>
#include <ul/ul_Result.hpp>

namespace ul::os {

    std::vector<cfg::TitleRecord> QueryInstalledTitles() {
        std::vector<cfg::TitleRecord> titles;

        // FIX: app record count?
        auto records_buf = new NsApplicationRecord[MaxTitleCount]();
        s32 record_count;
        UL_RC_ASSERT(nsListApplicationRecord(records_buf, MaxTitleCount, 0, &record_count));
        for(s32 i = 0; i < record_count; i++) {
            const auto &record = records_buf[i];
            if(record.application_id == 0) {
                continue;
            }
            const cfg::TitleRecord rec = {
                .title_type = cfg::TitleType::Installed,
                .app_id = record.application_id
            };
            titles.push_back(std::move(rec));
        }
        delete[] records_buf;
        return titles;
    }

}