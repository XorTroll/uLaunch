#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <fs/fs_Stdio.hpp>
#include <am/am_LibraryApplet.hpp>

namespace dmi {

    namespace impl {

        #define _UL_DMI_IMPL_RC_TRY_LOOP(...) ({ \
            auto rc = ResultSuccess; \
            do { \
                __VA_ARGS__ \
                svcSleepThread(10'000'000); \
            } while(R_FAILED(rc)); \
            return rc; \
        })

        #define _UL_DMI_IMPL_RC_TRY(...) ({ \
            auto rc = ResultSuccess; \
            __VA_ARGS__ \
            return rc; \
        })

        #define _UL_DMI_IMPL_RC(...) ({ \
            if(wait) { \
                _UL_DMI_IMPL_RC_TRY_LOOP( __VA_ARGS__ ); \
            } \
            else { \
                _UL_DMI_IMPL_RC_TRY( __VA_ARGS__ ); \
            } \
            return ResultSuccess; \
        })

        Result DaemonWriteImpl(void *data, size_t size, bool wait) {
            _UL_DMI_IMPL_RC(
                rc = am::LibraryAppletSend(data, size);
            );
        }
        
        Result DaemonReadImpl(void *data, size_t size, bool wait) {
            _UL_DMI_IMPL_RC(
                rc = am::LibraryAppletRead(data, size);
            );
        }

        Result MenuWriteImpl(void *data, size_t size, bool wait) {
            _UL_DMI_IMPL_RC(
                AppletStorage st;
                rc = appletCreateStorage(&st, size);
                if(R_SUCCEEDED(rc)) {
                    rc = appletStorageWrite(&st, 0, data, size);
                    if(R_SUCCEEDED(rc)) {
                        rc = appletPushOutData(&st);
                    }
                    appletStorageClose(&st);
                }
            );
        }

        Result MenuReadImpl(void *data, size_t size, bool wait) {
            _UL_DMI_IMPL_RC(
                AppletStorage st;
                rc = appletPopInData(&st);
                if(R_SUCCEEDED(rc)) {
                    rc = appletStorageRead(&st, 0, data, size);
                    appletStorageClose(&st);
                }
            );
        }

    }

}