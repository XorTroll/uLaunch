
#pragma once
#include <q_Include.hpp>
#include <stratosphere.hpp>

namespace ipc
{
    class IDaemonService : public IServiceObject
    {
        private:

            enum class CommandId
            {
                GetLatestMessage = 0
            };

        public:

            Result GetLatestMessage(Out<u32> msg);

        public:

            DEFINE_SERVICE_DISPATCH_TABLE
            {
                MAKE_SERVICE_COMMAND_META(IDaemonService, GetLatestMessage)
            };
    };
}