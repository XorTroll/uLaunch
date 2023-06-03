
#pragma once
#include <ul/loader/loader_TargetInput.hpp>

namespace ul::loader {

    Result Target(const TargetInput &target_ipt, const bool is_auto_gameplay_recording, const u64 applet_heap_size, const u64 applet_heap_reservation_size);

}