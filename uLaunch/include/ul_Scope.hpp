
#pragma once
#include <functional>

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

#define UL_ON_SCOPE_EXIT(...) OnScopeExit UL_UNIQUE_VAR_NAME(on_scope_exit) ([&]() { __VA_ARGS__ })