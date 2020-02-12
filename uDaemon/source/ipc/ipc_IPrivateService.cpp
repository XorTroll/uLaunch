#include <ipc/ipc_IPrivateService.hpp>
#include <am/am_DaemonMenuInteraction.hpp>
#include <am/am_LibraryApplet.hpp>

extern ams::os::Mutex g_last_menu_msg_lock;
extern am::MenuMessage g_last_menu_msg;

namespace ipc
{
    ams::Result IPrivateService::GetLatestMessage(ams::sf::Out<u32> msg, const ams::sf::ClientProcessId &client_pid)
    {
        u64 program_id = 0;
        UL_ASSERT(pminfoGetProgramId(&program_id, client_pid.process_id.value));
        
        auto last_menu_program_id = am::LibraryAppletGetProgramIdForAppletId(am::LibraryAppletGetMenuAppletId());
        // If Menu hasn't been launched it's program ID will be 0/invalid, thus a != check wouldn't be enough
        // If any of the IDs is invalid, something unexpected is happening...
        if((last_menu_program_id == 0) || (program_id == 0) || (program_id != last_menu_program_id)) return RES_VALUE(Daemon, PrivateServiceInvalidProcess);
        
        std::scoped_lock _lock(g_last_menu_msg_lock);
        msg.SetValue((u32)g_last_menu_msg);
        g_last_menu_msg = am::MenuMessage::Invalid;

        return ams::ResultSuccess();
    }
}