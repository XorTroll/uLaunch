#include <util/util_Convert.hpp>

namespace util {

    std::string Format128NintendoStyle(const AccountUid value) {
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

    std::string FormatApplicationId(const u64 app_id) {
        std::stringstream strm;
        strm << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << app_id;
        return strm.str();
    }

    std::string FormatResultDisplay(const Result rc) {
        char res[0x20] = {};
        sprintf(res, "%04d-%04d", R_MODULE(rc) + 2000, R_DESCRIPTION(rc));   
        return res;
    }

    std::string FormatResultHex(const Result rc) {
        char res[0x20] = {};
        sprintf(res, "0x%X", rc);
        return res;
    }

    std::string FormatResult(const Result rc) {
        const auto rc_mod = res::GetModuleByResult(rc);
        const auto rc_name = res::GetNameByResult(rc);
        const auto rc_fmt = FormatResultDisplay(rc);
        return rc_mod + " - " + rc_name + " (" + rc_fmt + ")";
    }

}