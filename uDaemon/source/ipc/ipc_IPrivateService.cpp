#include <ipc/ipc_IPrivateService.hpp>
#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <am/am_LibraryApplet.hpp>

extern ams::os::Mutex g_LastMenuMessageLock;
extern dmi::MenuMessage g_LastMenuMessage;

namespace ipc {

    ams::Result PrivateService::Initialize(const ams::sf::ClientProcessId &client_pid) {
        if(!this->initialized) {
            u64 program_id = 0;
            R_TRY(pminfoGetProgramId(&program_id, client_pid.process_id.value));
            
            auto last_menu_program_id = am::LibraryAppletGetProgramIdForAppletId(am::LibraryAppletGetMenuAppletId());
            // If Menu hasn't been launched it's program ID will be 0/invalid, thus a != check wouldn't be enough
            // If any of the IDs is invalid, something unexpected is happening...
            if((last_menu_program_id == 0) || (program_id == 0) || (program_id != last_menu_program_id)) {
                return RES_VALUE(Daemon, PrivateServiceInvalidProcess);
            }
            
            this->initialized = true;
        }
        
        return ams::ResultSuccess();
    }

    ams::Result PrivateService::GetMessage(ams::sf::Out<dmi::MenuMessage> out_msg) {
        if(!this->initialized) {
            return RES_VALUE(Daemon, PrivateServiceInvalidProcess);
        }

        std::scoped_lock lk(g_LastMenuMessageLock);
        out_msg.SetValue(g_LastMenuMessage);
        auto f = fopen("sdmc:/udaemon-priv.log", "ab+");
        if(f) {
            auto msg = "Sending message: " + std::to_string(static_cast<u32>(g_LastMenuMessage));
            fwrite(msg.c_str(), msg.length(), 1, f);
            fclose(f);
        }
        g_LastMenuMessage = dmi::MenuMessage::Invalid;
        return ams::ResultSuccess();
    }

}