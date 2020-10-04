
#pragma once
#include <stratosphere.hpp>
#include <ul_Include.hpp>

namespace ipc {

    namespace {

        using namespace ams;
        namespace os = ams::os; // Solves os:: namespace ambiguity (ams::os and uLaunch's os)

        #define PRIVATE_SERVICE_INTERFACE_INFO(C, H) \
            AMS_SF_METHOD_INFO(C, H,  0, ams::Result, GetLatestMessage, (const ams::sf::ClientProcessId &client_pid, ams::sf::Out<u32> out_msg))

        AMS_SF_DEFINE_INTERFACE(IPrivateService, PRIVATE_SERVICE_INTERFACE_INFO)

    }

    class PrivateService final {
        public:
            ams::Result GetLatestMessage(const ams::sf::ClientProcessId &client_pid, ams::sf::Out<u32> out_msg);
    };
    static_assert(IsIPrivateService<PrivateService>);

}