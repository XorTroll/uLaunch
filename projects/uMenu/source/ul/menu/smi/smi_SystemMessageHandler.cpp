#include <ul/menu/smi/smi_SystemMessageHandler.hpp>
#include <ul/sf/sf_Base.hpp>
#include <ul/util/util_Scope.hpp>
#include <atomic>

namespace ul::menu::smi {

    namespace {

        inline Result privateServiceInitialize(Service *srv) {
            u64 pid_placeholder = 0;
            return serviceDispatchIn(srv, 0, pid_placeholder,
                .in_send_pid = true
            );
        }

        inline Result privateServicePopMessageContext(Service *srv, MenuMessageContext *out_msg_ctx) {
            return serviceDispatchOut(srv, 1, *out_msg_ctx);
        }

        Service g_PrivateService;

        Result InitializePrivateService() {
            if(serviceIsActive(&g_PrivateService)) {
                return ResultSuccess;
            }

            UL_RC_TRY(smGetService(&g_PrivateService, sf::PrivateServiceName));
            UL_RC_TRY(privateServiceInitialize(&g_PrivateService));

            return ResultSuccess;
        }

        void FinalizePrivateService() {
            serviceClose(&g_PrivateService);
        }

        Result PopPrivateServiceMessageContext(MenuMessageContext *out_msg_ctx) {
            return privateServicePopMessageContext(&g_PrivateService, out_msg_ctx);
        }

    }

    namespace {

        bool g_Initialized = false;
        std::atomic_bool g_ReceiverThreadShouldStop = false;
        Thread g_ReceiverThread;
        std::vector<std::pair<OnMessageCallback, MenuMessage>> g_MessageCallbackTable;
        Mutex g_CallbackTableLock = {};

        void SystemMessageReceiverThread(void*) {
            while(true) {
                if(g_ReceiverThreadShouldStop) {
                    break;
                }

                {
                    MenuMessageContext last_msg_ctx;
                    if(R_SUCCEEDED(PopPrivateServiceMessageContext(&last_msg_ctx))) {
                        util::ScopedLock lk(g_CallbackTableLock);

                        for(const auto &[cb, msg] : g_MessageCallbackTable) {
                            if((msg == MenuMessage::Invalid) || (msg == last_msg_ctx.msg)) {
                                cb(last_msg_ctx);
                            }
                        }
                    }
                    
                }

                svcSleepThread(10'000'000ul);
            }
        }

    }

    Result InitializeSystemMessageHandler() {
        if(g_Initialized) {
            return ResultSuccess;
        }

        UL_RC_TRY(InitializePrivateService());

        g_ReceiverThreadShouldStop = false;
        UL_RC_TRY(threadCreate(&g_ReceiverThread, &SystemMessageReceiverThread, nullptr, nullptr, 0x1000, 49, -2));
        UL_RC_TRY(threadStart(&g_ReceiverThread));

        g_Initialized = true;
        return ResultSuccess;
    }

    void FinalizeSystemMessageHandler() {
        if(!g_Initialized) {
            return;
        }
        
        g_ReceiverThreadShouldStop = true;
        threadWaitForExit(&g_ReceiverThread);
        threadClose(&g_ReceiverThread);

        FinalizePrivateService();
        g_Initialized = false;
    }

    void RegisterOnMessageDetect(OnMessageCallback callback, const MenuMessage desired_msg) {
        util::ScopedLock lk(g_CallbackTableLock);

        g_MessageCallbackTable.push_back({ callback, desired_msg });
    }

}