#include <os/os_Titles.hpp>
#include <db/db_Save.hpp>
#include <fs/fs_Stdio.hpp>

namespace os
{
    ResultWith<std::vector<cfg::TitleRecord>> QueryInstalledTitles(bool cache)
    {
        std::vector<cfg::TitleRecord> titles;
        NsApplicationRecord *recordbuf = new NsApplicationRecord[OS_MAX_TITLE_COUNT]();
        size_t record_count = 0;
        R_TRY_WITH(nsListApplicationRecord(recordbuf, OS_MAX_TITLE_COUNT * sizeof(NsApplicationRecord), 0, &record_count), titles);
        for(u32 i = 0; i < record_count; i++)
        {
            cfg::TitleRecord rec = {};
            rec.app_id = recordbuf[i].titleID;
            rec.title_type = (u32)cfg::TitleType::Installed;
            if(rec.app_id == 0) continue;
            if(cache)
            {
                NsApplicationControlData control = {};
                size_t dummy;
                auto rc = nsGetApplicationControlData(1, rec.app_id, &control, sizeof(NsApplicationControlData), &dummy);
                if(R_SUCCEEDED(rc))
                {
                    auto fname = cfg::GetTitleCacheIconPath(rec.app_id);
                    fs::DeleteFile(fname);
                    db::Commit();
                    fs::WriteFile(fname, control.icon, sizeof(control.icon), true);
                }
            }
            titles.push_back(rec);
        }
        delete[] recordbuf;
        return SuccessResultWith(titles);
    }
}