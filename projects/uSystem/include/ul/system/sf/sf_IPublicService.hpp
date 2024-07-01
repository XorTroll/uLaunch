
#pragma once
#include <stratosphere.hpp>
#include <ul/system/smi/smi_SystemProtocol.hpp>

#define UL_SYSTEM_SF_I_PUBLIC_SERVICE_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, GetVersion, (::ams::sf::Out<::ul::Version> out_ver), (out_ver))

AMS_SF_DEFINE_INTERFACE(ams::ul::system::sf, IPublicService, UL_SYSTEM_SF_I_PUBLIC_SERVICE_INTERFACE_INFO, 0xCAFEBEEF)

namespace ul::system::sf {

    class PublicService {
        public:
            ::ams::Result GetVersion(::ams::sf::Out<Version> out_ver);
    };
    static_assert(::ams::ul::system::sf::IsIPublicService<PublicService>);

}
