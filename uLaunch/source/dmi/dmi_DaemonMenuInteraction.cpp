#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <fs/fs_Stdio.hpp>
#include <am/am_LibraryApplet.hpp>

namespace dmi {

    namespace {

        constexpr u32 MaxRetryCount = 10000;
        
        inline Result LoopWait(Result(*cmd_fn)(AppletStorage*), AppletStorage *st, const bool wait) {
            if(!wait) {
                return cmd_fn(st);
            }

            u32 count = 0;
            while(true) {
                if(R_SUCCEEDED(cmd_fn(st))) {
                    break;
                }

                count++;
                if(count > MaxRetryCount) {
                    return ResultWaitTimeout;
                }

                svcSleepThread(10'000'000);
            }

            return ResultSuccess;
        }

    }

    namespace dmn {

        Result PushStorage(AppletStorage *st) {
            return LoopWait(&am::LibraryAppletPush, st, false);
        }

        Result PopStorage(AppletStorage *st, const bool wait) {
            return LoopWait(&am::LibraryAppletPop, st, wait);
        }

    }

    namespace menu {

        Result PushStorage(AppletStorage *st) {
            return LoopWait(&appletPushOutData, st, false);
        }

        Result PopStorage(AppletStorage *st, const bool wait) {
            return LoopWait(&appletPopInData, st, wait);
        }

    }

}