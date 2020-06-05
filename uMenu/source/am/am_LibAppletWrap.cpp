#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <am/am_LibAppletWrap.hpp>
#include <am/am_DaemonMessages.hpp>

static Mutex g_amwrap_detection_lock = EmptyMutex;
static bool g_is_applet_running = false;
static bool g_detection_home_pressed = false;

namespace am {

    void OnHomeButtonDetection() {
        mutexLock(&g_amwrap_detection_lock);
        if(g_is_applet_running) {
            g_detection_home_pressed = true;
        }
        mutexUnlock(&g_amwrap_detection_lock);
    }

    void RegisterLibAppletHomeButtonDetection() {
        RegisterOnMessageDetect(&OnHomeButtonDetection, dmi::MenuMessage::HomeRequest);
    }

}

extern "C" {

    // Wrap libappletStart and libappletLaunch to use our custom waiting system
    Result __wrap_libappletStart(AppletHolder *h) {
        R_TRY(appletHolderStart(h));

        mutexLock(&g_amwrap_detection_lock);
        g_is_applet_running = true;
        mutexUnlock(&g_amwrap_detection_lock);

        while(true) {
            if(appletHolderCheckFinished(h)) {
                break;
            }
            if(!serviceIsActive(&h->s)) {
                break;
            }

            mutexLock(&g_amwrap_detection_lock);
            auto home_pressed = g_detection_home_pressed;
            g_detection_home_pressed = false;
            mutexUnlock(&g_amwrap_detection_lock);

            if(home_pressed) {
                appletHolderRequestExitOrTerminate(h, 15'000'000'000ul);
                break;
            }
            svcSleepThread(10'000'000ul);
        }

        appletHolderJoin(h);

        auto rc = ResultSuccess;

        auto reason = appletHolderGetExitReason(h);
        if(reason == LibAppletExitReason_Canceled || reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
            rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        }

        mutexLock(&g_amwrap_detection_lock);
        g_is_applet_running = false;
        mutexUnlock(&g_amwrap_detection_lock);

        return rc;
    }

    Result __wrap_libappletLaunch(AppletId id, LibAppletArgs *commonargs, const void* arg, size_t arg_size, void* reply, size_t reply_size, size_t *out_reply_size) {
        AppletHolder holder;
        R_TRY(appletCreateLibraryApplet(&holder, id, LibAppletMode_AllForeground));
        UL_ON_SCOPE_EXIT({
            appletHolderClose(&holder);
        });
        R_TRY(libappletArgsPush(commonargs, &holder));
        if(arg && arg_size) {
            R_TRY(libappletPushInData(&holder, arg, arg_size));
        }
        R_TRY(__wrap_libappletStart(&holder));
        if(reply && reply_size) {
            R_TRY(libappletPopOutData(&holder, reply, reply_size, out_reply_size));
        }
        return ResultSuccess;
    }

}