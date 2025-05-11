
#pragma once
#include <ul/loader/loader_TargetTypes.hpp>

namespace ul::loader {

    Result ReadTargetInput(TargetInput &out_target_ipt);
    Result WriteTargetOutput(const TargetOutput &target_opt);

}