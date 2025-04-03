#include <ul/man/man_Manager.hpp>
#include <ul/ul_Include.hpp>
#include <ul/fs/fs_Stdio.hpp>

namespace ul::man {

    bool IsBasePresent() {
        return fs::ExistsDirectory(BaseSystemPath);
    }

    bool IsSystemActive() {
        return fs::ExistsDirectory(ActiveSystemPath);
    }

    void ActivateSystem() {
        fs::CopyDirectory(BaseSystemPath, ActiveSystemPath);
    }

    void DeactivateSystem() {
        fs::DeleteDirectory(ActiveSystemPath);
    }

    std::string Version::AsString() const {
        auto as_str = std::to_string(this->major) + "." + std::to_string(this->minor);
        if(this->micro > 0) {
            as_str += "." + std::to_string(this->micro);
        }
        return as_str;
    }

    Version Version::FromString(const std::string &ver_str) {
        auto ver_str_cpy = ver_str;
        Version v = {};
        size_t pos = 0;
        std::string token;
        u32 c = 0;
        std::string delimiter = ".";
        while((pos = ver_str_cpy.find(delimiter)) != std::string::npos) {
            token = ver_str_cpy.substr(0, pos);
            if(c == 0) {
                v.major = std::stoi(token);
            }
            else if(c == 1) {
                v.minor = std::stoi(token);
            }
            else if(c == 2) {
                v.micro = std::stoi(token);
            }
            ver_str_cpy.erase(0, pos + delimiter.length());
            c++;
        }

        if(c == 0) {
            v.major = std::stoi(ver_str_cpy);
        }
        else if(c == 1) {
            v.minor = std::stoi(ver_str_cpy);
        }
        else if(c == 2) {
            v.micro = std::stoi(ver_str_cpy);
        }
        return v;
    }

}
