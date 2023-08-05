#include <ul/os/os_Applications.hpp>
#include <ul/ul_Result.hpp>

namespace ul::os {

    std::vector<NsApplicationRecord> ListApplicationRecords() {
        std::vector<NsApplicationRecord> records;

        auto records_buf = new NsApplicationRecord[MaxApplicationCount]();
        s32 record_count;
        UL_RC_ASSERT(nsListApplicationRecord(records_buf, MaxApplicationCount, 0, &record_count));
        for(s32 i = 0; i < record_count; i++) {
            const auto &record = records_buf[i];
            if(record.application_id != 0) {
                records.push_back(record);
            }
        }
        delete[] records_buf;
        return records;
    }

    NsApplicationContentMetaStatus GetApplicationContentMetaStatus(const u64 app_id) {
        s32 tmp_count;
        NsApplicationContentMetaStatus status;
        UL_RC_ASSERT(nsListApplicationContentMetaStatus(app_id, 0, &status, 1, &tmp_count));
        UL_ASSERT_TRUE(tmp_count > 0);
        return status;
    }

}