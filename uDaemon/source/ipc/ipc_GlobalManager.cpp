#include <ipc/ipc_GlobalManager.hpp>
#include <ipc/ipc_IPrivateService.hpp>
// IPublicService...

namespace {

    ipc::GlobalManager g_GlobalManager;
    ams::os::ThreadType g_GlobalManagerThread;
    alignas(ams::os::ThreadStackAlignment) u8 g_GlobalManagerThreadStack[0x4000];
    // ams::os::Mutex g_GlobalManagerLock(false);

    void GlobalManagerThread(void *_arg) {
        UL_ASSERT((g_GlobalManager.RegisterServer<ipc::IPrivateService, ipc::PrivateService>(ipc::PrivateServiceName, ipc::MaxPrivateSessions).GetValue()));
        // UL_ASSERT(g_GlobalManager.RegisterServer<ipc::IPublicService>(PublicServiceName, MaxPublicSessions).GetValue());

        g_GlobalManager.LoopProcess();
    }

}

namespace ipc {

    ::Result Initialize() {
        R_TRY(ams::os::CreateThread(&g_GlobalManagerThread, &GlobalManagerThread, nullptr, g_GlobalManagerThreadStack, sizeof(g_GlobalManagerThreadStack), 10).GetValue());
        ams::os::StartThread(&g_GlobalManagerThread);

        return ::ResultSuccess;
    }

    GlobalManager &GetGlobalManager() {
        return g_GlobalManager;
    }

}