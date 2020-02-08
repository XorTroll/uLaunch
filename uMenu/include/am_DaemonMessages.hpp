
#pragma once
#include <am/am_DaemonMenuInteraction.hpp>
#include <functional>

namespace am
{
    using MessageDetectCallback = std::function<void()>;

    Result InitializeDaemonMessageHandler();
    void ExitDaemonMessageHandler();
    void RegisterOnMessageDetect(MessageDetectCallback callback, MenuMessage desired_msg);
}