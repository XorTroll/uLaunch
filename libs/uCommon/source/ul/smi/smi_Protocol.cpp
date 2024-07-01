#include <ul/smi/smi_Protocol.hpp>

namespace ul::smi {

    namespace {

        constexpr u32 MaxRetryCount = 10000;

    }

    namespace impl {

        Result LoopWaitStorageFunctionImpl(StorageFunction st_fn, AppletStorage *st, const bool wait) {
            if(!wait) {
                return st_fn(st);
            }

            u32 count = 0;
            while(true) {
                if(R_SUCCEEDED(st_fn(st))) {
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

}
