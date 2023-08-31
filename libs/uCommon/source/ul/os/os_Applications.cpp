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

    Result GetApplicationContentMetaStatus(const u64 app_id, NsApplicationContentMetaStatus &out_status) {
        s32 tmp_count;
        UL_RC_TRY(nsListApplicationContentMetaStatus(app_id, 0, std::addressof(out_status), 1, &tmp_count));
        return ResultSuccess;
    }

}