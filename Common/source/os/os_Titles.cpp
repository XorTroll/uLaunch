#include <os/os_Titles.hpp>
#include <db/db_Save.hpp>
#include <fs/fs_Stdio.hpp>

namespace os
{
    std::vector<cfg::TitleRecord> QueryInstalledTitles()
    {
        std::vector<cfg::TitleRecord> titles;
        NsApplicationRecord *recordbuf = new NsApplicationRecord[OS_MAX_TITLE_COUNT]();
        s32 record_count = 0;
        auto rc = nsListApplicationRecord(recordbuf, OS_MAX_TITLE_COUNT, 0, &record_count);
        if(R_SUCCEEDED(rc))
        {
            for(s32 i = 0; i < record_count; i++)
            {
                cfg::TitleRecord rec = {};
                rec.app_id = recordbuf[i].application_id;
                rec.title_type = (u32)cfg::TitleType::Installed;
                if(rec.app_id == 0) continue;
                titles.push_back(rec);
            }
        }
        delete[] recordbuf;
        return titles;
    }
}