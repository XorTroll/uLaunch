#include <am/am_DaemonMessages.hpp>

namespace {

    Result daemonPrivateInitialize(Service *srv) {
        u64 pid_placeholder = 0;
        return serviceDispatchIn(srv, 0, pid_placeholder,
            .in_send_pid = true
        );
    }

    Result daemonPrivateGetMessage(Service *srv, dmi::MenuMessage *out_msg) {
        return serviceDispatchOut(srv, 1, *out_msg);
    }

    Service g_DaemonPrivateService;

    Result daemonInitializePrivateService() {
        if(serviceIsActive(&g_DaemonPrivateService)) {
            return ResultSuccess;
        }

        UL_RC_TRY(smGetService(&g_DaemonPrivateService, PrivateServiceName));
        UL_RC_TRY(daemonPrivateInitialize(&g_DaemonPrivateService));

        return ResultSuccess;
    }

    void daemonFinalizePrivateService() {
        serviceClose(&g_DaemonPrivateService);
    }

    dmi::MenuMessage daemonPrivateServiceGetMessage() {
        auto msg = dmi::MenuMessage::Invalid;
        UL_RC_ASSERT(daemonPrivateGetMessage(&g_DaemonPrivateService, &msg));
        return msg;
    }

}

namespace am {

    namespace {

        bool g_Initialized = false;
        std::atomic_bool g_ReceiveThreadShouldStop = false;
        Thread g_ReceiverThread;
        std::vector<std::pair<OnMessageCallback, dmi::MenuMessage>> g_MessageCallbackTable;
        Mutex g_CallbackTableLock = {};

        void DaemonMessageReceiverThread(void*) {
            while(true) {
                if(g_ReceiveThreadShouldStop) {
                    break;
                }

                {
                    const auto last_msg = daemonPrivateServiceGetMessage();
                    ScopedLock lk(g_CallbackTableLock);

                    for(const auto &[cb, msg] : g_MessageCallbackTable) {
                        if(msg == last_msg) {
                            cb();
                        }
                    }
                }

                svcSleepThread(10'000'000ul);
            }
        }

    }

    Result InitializeDaemonMessageHandler() {
        if(g_Initialized) {
            return ResultSuccess;
        }
        UL_RC_TRY(daemonInitializePrivateService());

        g_ReceiveThreadShouldStop = false;
        UL_RC_TRY(threadCreate(&g_ReceiverThread, &DaemonMessageReceiverThread, nullptr, nullptr, 0x1000, 49, -2));
        UL_RC_TRY(threadStart(&g_ReceiverThread));

        g_Initialized = true;
        return ResultSuccess;
    }

    void ExitDaemonMessageHandler() {
        if(!g_Initialized) {
            return;
        }
        
        g_ReceiveThreadShouldStop = true;
        threadWaitForExit(&g_ReceiverThread);
        threadClose(&g_ReceiverThread);

        daemonFinalizePrivateService();
        g_Initialized = false;
    }

    void RegisterOnMessageDetect(OnMessageCallback callback, const dmi::MenuMessage desired_msg) {
        ScopedLock lk(g_CallbackTableLock);

        g_MessageCallbackTable.push_back({ callback, desired_msg });
    }

}