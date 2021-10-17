#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <fs/fs_Stdio.hpp>
#include <am/am_LibraryApplet.hpp>

namespace dmi {

    namespace {

        constexpr u32 MaxRetryCount = 10000;
        
        inline Result LoopWait(Result(*cmd_fn)(AppletStorage*), AppletStorage *st, bool wait) {
            if(!wait) {
                return cmd_fn(st);
            }

            u32 count = 0;

            while(true) {
                auto rc = cmd_fn(st);
                if(R_SUCCEEDED(rc)) {
                    break;
                }

                count++;
                if(count > MaxRetryCount) {
                    return 0xCAFA;
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

        Result PopStorage(AppletStorage *st, bool wait) {
            return LoopWait(&am::LibraryAppletPop, st, wait);
        }

    }

    namespace menu {

        Result PushStorage(AppletStorage *st) {
            return LoopWait(&appletPushOutData, st, false);
        }

        Result PopStorage(AppletStorage *st, bool wait) {
            return LoopWait(&appletPopInData, st, wait);
        }

    }

}