#include <ul/os/os_Applications.hpp>
#include <ul/ul_Result.hpp>

namespace ul::os {

    namespace {

        constexpr size_t ApplicationRecordBufferCount = 30;
        NsApplicationRecord g_ApplicationRecordBuffer[ApplicationRecordBufferCount];

    }

    std::vector<NsApplicationRecord> ListApplicationRecords() {
        std::vector<NsApplicationRecord> records;

        s32 cur_offset = 0;
        while(true) {
            s32 record_count = 0;
            if(R_FAILED(nsListApplicationRecord(g_ApplicationRecordBuffer, ApplicationRecordBufferCount, cur_offset, &record_count))) {
                break;
            }
            if(record_count == 0) {
                break;
            }

            cur_offset += record_count;
            for(s32 i = 0; i < record_count; i++) {
                const auto &record = g_ApplicationRecordBuffer[i];
                if(record.application_id != 0) {
                    records.push_back(record);
                }
            }
        }

        return records;
    }

    Result GetApplicationContentMetaStatus(const u64 app_id, NsApplicationContentMetaStatus &out_status) {
        s32 tmp_count;
        UL_RC_TRY(nsListApplicationContentMetaStatus(app_id, 0, std::addressof(out_status), 1, &tmp_count));
        return ResultSuccess;
    }

}