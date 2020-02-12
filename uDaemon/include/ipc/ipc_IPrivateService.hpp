
#pragma once
#include <stratosphere.hpp>
#include <ul_Include.hpp>

namespace ipc
{
    class IPrivateService : public ams::sf::IServiceObject
    {
        private:

            enum class CommandId
            {
                GetLatestMessage = 0
            };

        public:

            ams::Result GetLatestMessage(ams::sf::Out<u32> msg, const ams::sf::ClientProcessId &client_pid);

        public:

            DEFINE_SERVICE_DISPATCH_TABLE
            {
                MAKE_SERVICE_COMMAND_META(GetLatestMessage)
            };
    };
}