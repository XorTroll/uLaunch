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

    std::vector<NsApplicationView> ListApplicationViews(const std::vector<NsApplicationRecord> &base_records) {
        const auto count = base_records.size();
        auto app_id_buf = new u64[count]();
        for(u32 i = 0; i < count; i++) {
            app_id_buf[i] = base_records.at(i).application_id;
        }

        auto app_view_buf = new NsApplicationView[count]();
        UL_RC_ASSERT(nsGetApplicationView(app_view_buf, app_id_buf, count));

        std::vector<NsApplicationView> views(app_view_buf, app_view_buf + count);

        delete[] app_view_buf;
        delete[] app_id_buf;
        return views;
    }

}
