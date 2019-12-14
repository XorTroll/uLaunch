#include <ipc/ipc_IDaemonService.hpp>
#include <am/am_QCommunications.hpp>

extern ams::os::Mutex latestqlock;
extern am::QMenuMessage latestqmenumsg;

namespace ipc
{
    void IDaemonService::GetLatestMessage(ams::sf::Out<u32> msg)
    {
        std::scoped_lock _lock(latestqlock);
        msg.SetValue((u32)latestqmenumsg);
        latestqmenumsg = am::QMenuMessage::Invalid;
    }
}