
#pragma once
#include <stratosphere.hpp>
#include <ul_Include.hpp>
#include <dmi/dmi_DaemonMenuInteraction.hpp>

namespace ipc {

    namespace {

        using namespace ams;
        namespace os = ams::os; // Solves os:: namespace ambiguity (ams::os and uLaunch's os)

        #define PRIVATE_SERVICE_INTERFACE_INFO(C, H) \
            AMS_SF_METHOD_INFO(C, H, 0, ams::Result, Initialize, (const ams::sf::ClientProcessId &client_pid)) \
            AMS_SF_METHOD_INFO(C, H, 1, ams::Result, GetMessage, (ams::sf::Out<dmi::MenuMessage> out_msg))

        AMS_SF_DEFINE_INTERFACE(IPrivateService, PRIVATE_SERVICE_INTERFACE_INFO)

    }

    class PrivateService final {
        private:
            bool initialized;
        public:
            PrivateService() : initialized(false) {}

            ams::Result Initialize(const ams::sf::ClientProcessId &client_pid);
            ams::Result GetMessage(ams::sf::Out<dmi::MenuMessage> out_msg);
    };
    static_assert(IsIPrivateService<PrivateService>);

}