#include <am_DaemonMessages.hpp>
#include <thread>
#include <map>

namespace am
{
    static Mutex g_receiver_lock = EmptyMutex;
    static Service g_daemon_srv;
    static bool g_init = false;
    static bool g_thr_should_stop = false;
    static Thread g_receiver_thr;
    static std::vector<std::pair<MessageDetectCallback, MenuMessage>> g_callback_table;

    static void DaemonMessageReceiveThread(void *arg)
    {
        while(true)
        {
            mutexLock(&g_receiver_lock);
            auto should_stop = g_thr_should_stop;
            mutexUnlock(&g_receiver_lock);
            if(should_stop) break;
            MenuMessage tmp_msg = MenuMessage::Invalid;
            u64 pid_placeholder = 0;
            UL_ASSERT(serviceDispatchInOut(&g_daemon_srv, 0, pid_placeholder, tmp_msg,
                .in_send_pid = true,
            ));
            mutexLock(&g_receiver_lock);
            for(auto &[cb, msg] : g_callback_table)
            {
                if(msg == tmp_msg)
                {
                    cb();
                }
            }
            mutexUnlock(&g_receiver_lock);
            svcSleepThread(10'000'000ul);
        }
    }

    Result InitializeDaemonMessageHandler()
    {
        if(g_init) return 0;
        auto rc = smGetService(&g_daemon_srv, AM_DAEMON_PRIVATE_SERVICE_NAME);
        if(R_SUCCEEDED(rc))
        {
            g_thr_should_stop = false;
            rc = threadCreate(&g_receiver_thr, &DaemonMessageReceiveThread, nullptr, nullptr, 0x4000, 0x2b, -2);
            if(R_SUCCEEDED(rc))
            {
                rc = threadStart(&g_receiver_thr);
                if(R_SUCCEEDED(rc)) g_init = true;
            }
        }
        return rc;
    }

    void ExitDaemonMessageHandler()
    {
        if(!g_init) return;
        mutexLock(&g_receiver_lock);
        g_thr_should_stop = true;
        mutexUnlock(&g_receiver_lock);
        threadWaitForExit(&g_receiver_thr);
        threadClose(&g_receiver_thr);
        serviceClose(&g_daemon_srv);
        g_init = false;
    }

    void RegisterOnMessageDetect(MessageDetectCallback callback, MenuMessage desired_msg)
    {
        mutexLock(&g_receiver_lock);
        g_callback_table.push_back(std::make_pair(callback, desired_msg));
        mutexUnlock(&g_receiver_lock);
    }

}