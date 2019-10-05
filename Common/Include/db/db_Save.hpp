
#pragma once
#include <q_Include.hpp>

namespace db
{
    static constexpr u64 HomeMenuSaveDataId = 0x8000000000001010;
    
    #define DB_INSIDE_BASE_DIR(path) std::string(Q_BASE_DB_DIR "/") + path
    
    struct PassBlock
    {
        u128 uid;
        char pass_sha[0x20]; // Password is stored as its SHA256
    } PACKED;

    static_assert(sizeof(PassBlock) == 0x30, "Password block must be size 0x40!");

    Result Mount();
    void Unmount();
    void Commit();

    ResultWith<PassBlock> AccessPassword(u128 user_id);
    std::string GetUserPasswordFilePath(u128 user_id);
    Result RegisterUserPassword(u128 user_id, std::string password);
    Result TryLogUser(u128 user_id, std::string password);
}