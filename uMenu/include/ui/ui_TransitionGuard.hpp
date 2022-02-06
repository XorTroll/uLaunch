
#pragma once
#include <ul_Include.hpp>

namespace ui {

    // Note: this guard ensures that certain code won't run again while we're already inside its invokation (which would happen when calling render loops)

    class TransitionGuard {
        private:
            bool on_transition;

        public:
            TransitionGuard() : on_transition(false) {}

            inline bool Run(std::function<void()> fn) {
                if(this->on_transition) {
                    return false;
                }

                this->on_transition = true;
                fn();
                this->on_transition = false;
                return true;
            }
    };

}