#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <am/am_LibnxLibappletWrap.hpp>
#include <am/am_DaemonMessages.hpp>

namespace {

    std::atomic_bool g_IsAppletRunning = false;
    std::atomic_bool g_DetectionHomePressed = false;

}

namespace am {

    void OnHomeButtonDetection() {
        if(g_IsAppletRunning) {
            g_DetectionHomePressed = true;
        }
    }

    void RegisterLibnxLibappletHomeButtonDetection() {
        RegisterOnMessageDetect(&OnHomeButtonDetection, dmi::MenuMessage::HomeRequest);
    }

}

extern "C" {

    // Wrap libappletStart and libappletLaunch to use our custom waiting system (mostly so that they respond to HOME menu presses)

    Result __wrap_libappletStart(AppletHolder *h) {
        UL_RC_TRY(appletHolderStart(h));

        g_IsAppletRunning = true;

        while(true) {
            if(appletHolderCheckFinished(h)) {
                break;
            }
            if(!serviceIsActive(&h->s)) {
                break;
            }

            const bool home_pressed = g_DetectionHomePressed;
            g_DetectionHomePressed = false;

            if(home_pressed) {
                appletHolderRequestExitOrTerminate(h, 15'000'000'000ul);
                break;
            }
            svcSleepThread(10'000'000ul);
        }

        appletHolderJoin(h);

        auto rc = ResultSuccess;
        const auto reason = appletHolderGetExitReason(h);
        if(reason == LibAppletExitReason_Canceled || reason == LibAppletExitReason_Abnormal || reason == LibAppletExitReason_Unexpected) {
            rc = MAKERESULT(Module_Libnx, LibnxError_LibAppletBadExit);
        }

        g_IsAppletRunning = false;

        return rc;
    }

    // Note: libappletLaunch is exactly the same but we redefine it so that it calls our libappletStart (libnx is already compiled, the libappletStart wrap wouldn't affect libappletLaunch otherwise)

    Result __wrap_libappletLaunch(AppletId id, LibAppletArgs *common_args, const void* arg, size_t arg_size, void* reply, size_t reply_size, size_t *out_reply_size) {
        AppletHolder holder;
        UL_RC_TRY(appletCreateLibraryApplet(&holder, id, LibAppletMode_AllForeground));
        UL_ON_SCOPE_EXIT({
            appletHolderClose(&holder);
        });
        UL_RC_TRY(libappletArgsPush(common_args, &holder));
        if(arg && arg_size) {
            UL_RC_TRY(libappletPushInData(&holder, arg, arg_size));
        }
        UL_RC_TRY(libappletStart(&holder));
        if(reply && reply_size) {
            UL_RC_TRY(libappletPopOutData(&holder, reply, reply_size, out_reply_size));
        }
        return ResultSuccess;
    }

}