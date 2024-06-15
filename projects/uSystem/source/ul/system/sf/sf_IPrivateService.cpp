#include <ul/system/sf/sf_IPrivateService.hpp>
#include <ul/system/la/la_LibraryApplet.hpp>
#include <queue>

extern ul::RecursiveMutex g_MenuMessageQueueLock;
extern std::queue<ul::smi::MenuMessageContext> *g_MenuMessageQueue;

namespace ul::system::sf {

    ::ams::Result PrivateService::Initialize(const ::ams::sf::ClientProcessId &client_pid) {
        if(!this->initialized) {
            u64 program_id = 0;
            UL_RC_TRY(pminfoInitialize());
            UL_RC_TRY(pminfoGetProgramId(&program_id, client_pid.process_id.value));
            pminfoExit();

            const auto last_menu_program_id = la::GetMenuProgramId();
            // If Menu hasn't been launched it's program ID will be 0 (invalid), thus a single (program_id != last_menu_program_id) check isn't enough
            // If any of the IDs is invalid, something unexpected is happening...
            if((last_menu_program_id == 0) || (program_id == 0) || (program_id != last_menu_program_id)) {
                return ResultInvalidProcess;
            }

            this->initialized = true;
        }
        
        return ResultSuccess;
    }

    ::ams::Result PrivateService::TryPopMessageContext(::ams::sf::Out<MenuMessageContext> out_msg_ctx) {
        if(!this->initialized) {
            return ResultInvalidProcess;
        }

        ScopedLock lk(g_MenuMessageQueueLock);
        if(g_MenuMessageQueue->empty()) {
            return ResultNoMessagesAvailable;
        }
        else {
            const auto last_msg_ctx = g_MenuMessageQueue->front();
            g_MenuMessageQueue->pop();
            out_msg_ctx.SetValue({ .actual_ctx = last_msg_ctx });
            return ResultSuccess;
        }
    }

}