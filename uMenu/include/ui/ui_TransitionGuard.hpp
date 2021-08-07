
#pragma once
#include <ul_Include.hpp>

namespace ui {

    class TransitionGuard {
        private:
            bool on_transition;

        public:
            TransitionGuard() : on_transition(false) {}

            bool Run(std::function<void()> fn) {
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