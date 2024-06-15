#include <ul/util/util_String.hpp>
#include <ul/ul_Result.hpp>

namespace ul::util {

    std::string FormatAccount(const AccountUid value) {
        auto bytes_v = reinterpret_cast<const u8*>(value.uid);
        std::stringstream strm;

        #define _UL_UTIL_PRINT(idx) strm << std::hex << std::setw(2) << std::setfill('0') << std::nouppercase << static_cast<const u32>(bytes_v[idx]);
        #define _UL_UTIL_DASH strm << "-";

        _UL_UTIL_PRINT(3)
        _UL_UTIL_PRINT(2)
        _UL_UTIL_PRINT(1)
        _UL_UTIL_PRINT(0)
        _UL_UTIL_DASH
        _UL_UTIL_PRINT(5)
        _UL_UTIL_PRINT(4)
        _UL_UTIL_DASH
        _UL_UTIL_PRINT(7)
        _UL_UTIL_PRINT(6)
        _UL_UTIL_DASH
        _UL_UTIL_PRINT(9)
        _UL_UTIL_PRINT(8)
        _UL_UTIL_DASH
        _UL_UTIL_PRINT(15)
        _UL_UTIL_PRINT(14)
        _UL_UTIL_PRINT(13)
        _UL_UTIL_PRINT(12)
        _UL_UTIL_PRINT(11)
        _UL_UTIL_PRINT(10)

        #undef _UL_UTIL_DASH
        #undef _UL_UTIL_PRINT

        return strm.str();
    }

    std::string FormatResultDisplay(const Result rc) {
        char res[0x40] = {};

        const char *mod_name;
        const char *rc_name;
        if(rc::GetResultNameAny(rc, mod_name, rc_name)) {
            sprintf(res, "%04d-%04d/0x%X/%s::%s", R_MODULE(rc) + 2000, R_DESCRIPTION(rc), R_VALUE(rc), mod_name, rc_name);
        }
        else {
            sprintf(res, "%04d-%04d/0x%X", R_MODULE(rc) + 2000, R_DESCRIPTION(rc), R_VALUE(rc));
        }
        
        return res;
    }

}