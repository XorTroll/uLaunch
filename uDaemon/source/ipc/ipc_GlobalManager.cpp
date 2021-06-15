#include <ipc/ipc_GlobalManager.hpp>
#include <ipc/ipc_IPrivateService.hpp>
// IPublicService...

namespace {

    ipc::ServerManager g_GlobalManager;
    ams::os::ThreadType g_GlobalManagerThread;
    alignas(ams::os::ThreadStackAlignment) u8 g_GlobalManagerThreadStack[0x8000];
    ams::os::Mutex g_ServerAllocatorLock(false);

    void GlobalManagerThread(void*) {
        UL_AMS_ASSERT(g_GlobalManager.RegisterServer(ipc::PortIndex_PrivateService, ipc::PrivateServiceName, ipc::MaxPrivateSessions));
        // UL_ASSERT(g_GlobalManager.RegisterServer<ipc::IPublicService>(PublicServiceName, MaxPublicSessions).GetValue());

        g_GlobalManager.LoopProcess();
    }

    alignas(0x40) constinit u8 g_server_allocator_buffer[0x8000];
    ams::lmem::HeapHandle g_server_heap_handle;
    ipc::Allocator g_server_allocator;

    void InitializeHeap() {
        g_server_heap_handle = ams::lmem::CreateExpHeap(g_server_allocator_buffer, sizeof(g_server_allocator_buffer), ams::lmem::CreateOption_None);
        g_server_allocator.Attach(g_server_heap_handle);
    }

}

namespace ipc {

    ams::Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
        switch(port_index) {
            case PortIndex_PrivateService: {
                return this->AcceptImpl(server, MakeShared<ams::sf::ul::IPrivateService, ipc::PrivateService>());
            }
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    ::Result Initialize() {
        InitializeHeap();
        R_TRY(ams::os::CreateThread(&g_GlobalManagerThread, &GlobalManagerThread, nullptr, g_GlobalManagerThreadStack, sizeof(g_GlobalManagerThreadStack), 10).GetValue());
        ams::os::StartThread(&g_GlobalManagerThread);

        return ::ResultSuccess;
    }

    ServerManager &GetGlobalManager() {
        return g_GlobalManager;
    }

    Allocator &GetServerAllocator() {
        std::scoped_lock lk(g_ServerAllocatorLock);
        return g_server_allocator;
    }

}