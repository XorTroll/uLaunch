#include <ipc/ipc_IDaemonService.hpp>
#include <am/am_DaemonMenuInteraction.hpp>

extern ams::os::Mutex g_last_menu_msg_lock;
extern am::MenuMessage g_last_menu_msg = am::MenuMessage::Invalid;

namespace ipc
{
    void IDaemonService::GetLatestMessage(ams::sf::Out<u32> msg)
    {
        std::scoped_lock _lock(g_last_menu_msg_lock);
        msg.SetValue((u32)g_last_menu_msg);
        g_last_menu_msg = am::MenuMessage::Invalid;
    }
}