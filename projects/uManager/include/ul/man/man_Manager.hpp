
#pragma once
#include <ul/ul_Include.hpp>

namespace ul::man {

    constexpr const char ActiveSystemPath[] = "sdmc:/atmosphere/contents/0100000000001000";
    constexpr const char BaseSystemPath[] = "sdmc:/ulaunch/bin/uSystem";

    bool IsBasePresent();
    bool IsSystemActive();

    void ActivateSystem();
    void DeactivateSystem();

    struct Version {
        u32 major;
        u32 minor;
        s32 micro;

        std::string AsString() const;

        static inline constexpr Version MakeVersion(const u32 major, const u32 minor, const u32 micro) {
            return { major, minor, static_cast<s32>(micro) };
        }

        static Version FromString(const std::string &ver_str);
        
        inline constexpr bool IsLower(const Version &other) const {
            if(this->major > other.major) {
                return true;
            }
            else if(this->major == other.major) {
                if(this->minor > other.minor) {
                    return true;
                }
                else if(this->minor == other.minor) {
                    if(this->micro > other.micro) {
                        return true;
                    }
                }
            }
            return false;
        }

        inline constexpr bool IsHigher(const Version &other) const {
            return !this->IsLower(other) && !this->IsEqual(other);
        }

        inline constexpr bool IsEqual(const Version &other) const {
            return ((this->major == other.major) && (this->minor == other.minor) && (this->micro == other.micro));
        }
    };

}