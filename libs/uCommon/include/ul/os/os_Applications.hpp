
#pragma once
#include <vector>
#include <ul/ul_Include.hpp>

namespace ul::os {

    constexpr u64 InvalidApplicationId = 0;

    template<NsExtApplicationViewFlag Flag>
    NX_CONSTEXPR bool ApplicationViewHasFlag(const NsExtApplicationView &view) {
        return nsextApplicationViewHasFlags(std::addressof(view), Flag);
    }

    std::vector<NsExtApplicationRecord> ListApplicationRecords();
    std::vector<NsExtApplicationView> ListApplicationViews(const std::vector<NsExtApplicationRecord> &base_records);

}
