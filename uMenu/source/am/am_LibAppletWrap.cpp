#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <am/am_LibAppletWrap.hpp>
#include <am/am_DaemonMessages.hpp>

namespace {

    Mutex g_LibAppletWrapLock = {};
    bool g_IsAppletRunning = false;
    bool g_DetectionHomePressed = false;

}

namespace am {

    void OnHomeButtonDetection() {
        mutexLock(&g_LibAppletWrapLock);
        if(g_IsAppletRunning) {
            g_DetectionHomePressed = true;
        }
        mutexUnlock(&g_LibAppletWrapLock);
    }

    void RegisterLibAppletHomeButtonDetection() {
        RegisterOnMessageDetect(&OnHomeButtonDetection, dmi::MenuMessage::HomeRequest);
    }

}

extern "C" {

    // Wrap libappletStart and libappletLaunch to use our custom waiting system

    Result __wrap_libappletStart(AppletHolder *h) {
        R_TRY(appletHolderStart(h));

        mutexLock(&g_LibAppletWrapLock);
        g_IsAppletRunning = true;
        mutexUnlock(&g_LibAppletWrapLock);

        while(true) {
            if(appletHolderCheckFinished(h)) {
                break;
            }
            if(!serviceIsActive(&h->s)) {
                break;
            }

            mutexLock(&g_LibAppletWrapLock);
            auto home_pressed = g_DetectionHomePressed;
            g_DetectionHomePressed = false;
            mutexUnlock(&g_LibAppletWrapLock);

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

        mutexLock(&g_LibAppletWrapLock);
        g_IsAppletRunning = false;
        mutexUnlock(&g_LibAppletWrapLock);

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