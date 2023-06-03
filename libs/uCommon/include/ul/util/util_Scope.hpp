
#pragma once
#include <switch.h>
#include <functional>

namespace ul::util {

    class OnScopeExit {
        public:
            using Fn = std::function<void()>;

        private:
            Fn exit_fn;

        public:
            OnScopeExit(Fn fn) : exit_fn(fn) {}

            ~OnScopeExit() {
                (this->exit_fn)();
            }
    };

    #define UL_CONCAT_IMPL(x,y) x##y
    #define UL_CONCAT(x,y) UL_CONCAT_IMPL(x,y)
    #define UL_UNIQUE_VAR_NAME(prefix) UL_CONCAT(prefix ## _, __COUNTER__)

    class ScopedLock {
        private:
            Mutex &lock;
        
        public:
            ScopedLock(Mutex &lock) : lock(lock) {
                mutexLock(std::addressof(lock));
            }

            ~ScopedLock() {
                mutexUnlock(std::addressof(this->lock));
            }
    };

}