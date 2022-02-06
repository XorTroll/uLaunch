#include <ipc/ipc_Manager.hpp>
#include <ipc/ipc_IPrivateService.hpp>

namespace {

    ipc::ServerManager g_Manager;

    constexpr size_t IpcManagerThreadStackSize = 32_KB;
    ams::os::ThreadType g_ManagerThread;
    alignas(ams::os::ThreadStackAlignment) u8 g_ManagerThreadStack[IpcManagerThreadStackSize];
    
    ams::os::Mutex g_ManagerAllocatorLock(false);

    constexpr size_t ServerAllocatorHeapSize = 32_KB;
    alignas(0x40) constinit u8 g_ManagerAllocatorHeap[ServerAllocatorHeapSize];
    ams::lmem::HeapHandle g_ManagerAllocatorHeapHandle;
    ipc::Allocator g_ManagerAllocator;

    void IpcManagerThread(void*) {
        ams::os::SetThreadNamePointer(ams::os::GetCurrentThread(), "ul.daemon.IpcManager");

        UL_RC_ASSERT(g_Manager.RegisterServer(ipc::Port_PrivateService, ipc::PrivateServiceName, ipc::MaxPrivateSessions));
        // UL_RC_ASSERT(g_Manager.RegisterServer<ipc::IPublicService>(PublicServiceName, MaxPublicSessions));

        g_Manager.LoopProcess();
    }

    void InitializeHeap() {
        g_ManagerAllocatorHeapHandle = ams::lmem::CreateExpHeap(g_ManagerAllocatorHeap, sizeof(g_ManagerAllocatorHeap), ams::lmem::CreateOption_None);
        g_ManagerAllocator.Attach(g_ManagerAllocatorHeapHandle);
    }

}

namespace ipc {

    ams::Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
        switch(port_index) {
            case Port_PrivateService: {
                return this->AcceptImpl(server, MakeShared<ams::sf::ul::IPrivateService, ipc::PrivateService>());
            }
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result Initialize() {
        InitializeHeap();
        UL_RC_TRY(ams::os::CreateThread(&g_ManagerThread, &IpcManagerThread, nullptr, g_ManagerThreadStack, sizeof(g_ManagerThreadStack), 10));
        ams::os::StartThread(&g_ManagerThread);

        return ResultSuccess;
    }

    Allocator &GetManagerAllocator() {
        std::scoped_lock lk(g_ManagerAllocatorLock);
        return g_ManagerAllocator;
    }

    ams::Result RegisterSession(const ams::os::NativeHandle session_handle, ams::sf::cmif::ServiceObjectHolder &&obj) {
        return g_Manager.RegisterSession(session_handle, std::move(obj));
    }

}