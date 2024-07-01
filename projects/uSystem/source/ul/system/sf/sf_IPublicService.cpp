#include <ul/system/sf/sf_IPublicService.hpp>

namespace ul::system::sf {

    ::ams::Result PublicService::GetVersion(::ams::sf::Out<Version> out_ver) {
        out_ver.SetValue(CurrentVersion);
        return ResultSuccess;
    }

}
