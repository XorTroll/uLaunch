#include <ul/os/os_Applications.hpp>
#include <ul/ul_Result.hpp>

namespace ul::os {

    namespace {

        constexpr size_t ApplicationRecordBufferCount = 30;
        NsExtApplicationRecord g_ApplicationRecordBuffer[ApplicationRecordBufferCount];

    }

    std::vector<NsExtApplicationRecord> ListApplicationRecords() {
        std::vector<NsExtApplicationRecord> records;

        s32 cur_offset = 0;
        while(true) {
            s32 record_count;
            if(R_FAILED(nsListApplicationRecord(reinterpret_cast<NsApplicationRecord*>(g_ApplicationRecordBuffer), ApplicationRecordBufferCount, cur_offset, &record_count))) {
                break;
            }
            if(record_count == 0) {
                break;
            }

            cur_offset += record_count;
            records.insert(records.end(), g_ApplicationRecordBuffer, g_ApplicationRecordBuffer + record_count);
        }

        return records;
    }

    std::vector<NsExtApplicationView> ListApplicationViews(const std::vector<NsExtApplicationRecord> &base_records) {
        const auto count = base_records.size();
        auto app_id_buf = new u64[count]();
        for(u32 i = 0; i < count; i++) {
            app_id_buf[i] = base_records.at(i).id;
        }

        auto app_view_buf = new NsExtApplicationView[count]();
        UL_RC_ASSERT(nsGetApplicationView(reinterpret_cast<NsApplicationView*>(app_view_buf), app_id_buf, count));

        std::vector<NsExtApplicationView> views(app_view_buf, app_view_buf + count);

        delete[] app_view_buf;
        delete[] app_id_buf;
        return views;
    }

}
