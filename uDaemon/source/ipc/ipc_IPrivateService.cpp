#include <ipc/ipc_IPrivateService.hpp>
#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <am/am_LibraryApplet.hpp>

extern ams::os::Mutex g_last_menu_msg_lock;
extern dmi::MenuMessage g_last_menu_msg;

namespace ipc {

    ams::Result IPrivateService::GetLatestMessage(const ams::sf::ClientProcessId &client_pid, ams::sf::Out<u32> out_msg) {
        u64 program_id = 0;
        R_TRY(pminfoGetProgramId(&program_id, client_pid.process_id.value));
        
        auto last_menu_program_id = am::LibraryAppletGetProgramIdForAppletId(am::LibraryAppletGetMenuAppletId());
        // If Menu hasn't been launched it's program ID will be 0/invalid, thus a != check wouldn't be enough
        // If any of the IDs is invalid, something unexpected is happening...
        if((last_menu_program_id == 0) || (program_id == 0) || (program_id != last_menu_program_id)) {
            return RES_VALUE(Daemon, PrivateServiceInvalidProcess);
        }
        
        std::scoped_lock lk(g_last_menu_msg_lock);
        out_msg.SetValue(static_cast<u32>(g_last_menu_msg));
        g_last_menu_msg = dmi::MenuMessage::Invalid;
        return ams::ResultSuccess();
    }

}