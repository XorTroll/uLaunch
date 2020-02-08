
#pragma once
#include <ul_Include.hpp>

namespace db
{
    static constexpr u64 HomeMenuSaveDataId = 0x8000000000001010;

    Result Mount();
    void Unmount();
    void Commit();
}