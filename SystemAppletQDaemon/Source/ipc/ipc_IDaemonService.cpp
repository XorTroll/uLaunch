#include <ipc/ipc_IDaemonService.hpp>
#include <am/am_QCommunications.hpp>

extern HosMutex latestqlock;
extern am::QMenuMessage latestqmenumsg;

namespace ipc
{
    Result IDaemonService::GetLatestMessage(Out<u32> msg)
    {
        std::scoped_lock lck(latestqlock);
        msg.SetValue((u32)latestqmenumsg);
        latestqmenumsg = am::QMenuMessage::Invalid;
        return 0;
    }
}