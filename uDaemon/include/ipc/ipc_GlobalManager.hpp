#include <stratosphere.hpp>
#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <functional>

namespace ipc {

    struct ServerOptions {
        static const size_t PointerBufferSize = 0x400;
        static const size_t MaxDomains = 0x40;
        static const size_t MaxDomainObjects = 0x40;
    };

    constexpr size_t MaxPrivateSessions = 1;
    constexpr ams::sm::ServiceName PrivateServiceName = ams::sm::ServiceName::Encode(AM_DAEMON_PRIVATE_SERVICE_NAME);

    constexpr size_t MaxPublicSessions = 0x20;
    constexpr ams::sm::ServiceName PublicServiceName = ams::sm::ServiceName::Encode(AM_DAEMON_PUBLIC_SERVICE_NAME);

    constexpr size_t NumServers = 2;
    constexpr size_t MaxSessions = MaxPrivateSessions + MaxPublicSessions + 1;

    using GlobalManager = ams::sf::hipc::ServerManager<NumServers, ServerOptions, MaxSessions>;

    Result Initialize();
    GlobalManager &GetGlobalManager();

}