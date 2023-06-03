
#pragma once
#include <stratosphere.hpp>
#include <ul/system/smi/smi_SystemProtocol.hpp>

#define UL_SYSTEM_SF_I_PRIVATE_SERVICE_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, Initialize, (const ams::sf::ClientProcessId &client_pid), (client_pid)) \
    AMS_SF_METHOD_INFO(C, H, 1, Result, PopMessage, (ams::sf::Out<::ul::smi::MenuMessage> out_msg), (out_msg))

AMS_SF_DEFINE_INTERFACE(ams::ul::system::sf, IPrivateService, UL_SYSTEM_SF_I_PRIVATE_SERVICE_INTERFACE_INFO, 0xCAFEBABE)

namespace ul::system::sf {

    class PrivateService {
        private:
            bool initialized;

        public:
            PrivateService() : initialized(false) {}

            ams::Result Initialize(const ams::sf::ClientProcessId &client_pid);
            ams::Result PopMessage(ams::sf::Out<smi::MenuMessage> out_msg);
    };
    static_assert(ams::ul::system::sf::IsIPrivateService<PrivateService>);

}