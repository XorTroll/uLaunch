
#pragma once
#include <stratosphere.hpp>
#include <ul_Include.hpp>

namespace ipc
{
    class IDaemonService : public ams::sf::IServiceObject
    {
        private:

            enum class CommandId
            {
                GetLatestMessage = 0
            };

        public:

            void GetLatestMessage(ams::sf::Out<u32> msg);

        public:

            DEFINE_SERVICE_DISPATCH_TABLE
            {
                MAKE_SERVICE_COMMAND_META(GetLatestMessage)
            };
    };
}