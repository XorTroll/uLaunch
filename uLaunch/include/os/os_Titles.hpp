
#pragma once
#include <ul_Include.hpp>
#include <cfg/cfg_Config.hpp>

namespace os {

    #define UL_OS_FOR_EACH_APP_RECORD(rec, ...) ({ \
        NsApplicationRecord rec = {}; \
        s32 offset = 0; \
        s32 count = 0; \
        while(true) { \
            auto rc = nsListApplicationRecord(&rec, 1, offset, &count); \
            if(R_FAILED(rc) || (count < 1)) { \
                break; \
            } \
            if(rec.application_id != 0) { \
                __VA_ARGS__ \
            } \
            offset++; \
            rec = {}; \
        } \
    })

    std::vector<cfg::TitleRecord> QueryInstalledTitles();

}