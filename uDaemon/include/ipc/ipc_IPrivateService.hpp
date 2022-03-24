
#pragma once
#include <stratosphere.hpp>
#include <ul_Include.hpp>
#include <dmi/dmi_DaemonMenuInteraction.hpp>

#define IPC_I_PRIVATE_SERVICE_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, Initialize, (const ClientProcessId &client_pid), (client_pid)) \
    AMS_SF_METHOD_INFO(C, H, 1, Result, GetMessage, (Out<dmi::MenuMessage> out_msg), (out_msg))

AMS_SF_DEFINE_INTERFACE(ams::sf::ul, IPrivateService, IPC_I_PRIVATE_SERVICE_INTERFACE_INFO, 0xCAFEBABE)

namespace ipc {

    class PrivateService {
        private:
            bool initialized;
        public:
            PrivateService() : initialized(false) {}

            ams::Result Initialize(const ams::sf::ClientProcessId &client_pid);
            ams::Result GetMessage(ams::sf::Out<dmi::MenuMessage> out_msg);
    };
    static_assert(ams::sf::ul::IsIPrivateService<PrivateService>);

}