#include <stratosphere.hpp>
#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <functional>

namespace ipc {

    // Note: domains and pointer buffer are required since ECS sessions will make use of them (like normal fs interfaces)

    using Allocator = ams::sf::ExpHeapAllocator;
    using ObjectFactory = ams::sf::ObjectFactory<ams::sf::ExpHeapAllocator::Policy>;

    struct ServerOptions {
        static const size_t PointerBufferSize = 0x800;
        static const size_t MaxDomains = 0x40;
        static const size_t MaxDomainObjects = 0x100;
    };

    enum PortIndex {
        PortIndex_PrivateService,
        /*
        PortIndex_PublicService,
        */

        PortIndex_Count
    };

    constexpr size_t MaxPrivateSessions = 1;
    constexpr ams::sm::ServiceName PrivateServiceName = ams::sm::ServiceName::Encode(AM_DAEMON_PRIVATE_SERVICE_NAME);

    /*
    constexpr size_t MaxPublicSessions = 0x20;
    constexpr ams::sm::ServiceName PublicServiceName = ams::sm::ServiceName::Encode(AM_DAEMON_PUBLIC_SERVICE_NAME);
    */

    constexpr size_t MaxEcsExtraSessions = 5;
    constexpr size_t MaxSessions = MaxPrivateSessions + MaxEcsExtraSessions;

    class ServerManager final : public ams::sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions> {
        private:
            virtual ams::Result OnNeedsToAccept(int port_index, Server *server) override;
    };

    Result Initialize();
    ServerManager &GetGlobalManager();
    Allocator &GetServerAllocator();

    template<typename Impl, typename T, typename ...Args>
    inline auto MakeShared(Args ...args) {
        return ObjectFactory::CreateSharedEmplaced<Impl, T>(std::addressof(GetServerAllocator()), args...);
    }

}