
#pragma once
#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <functional>

namespace am {

    using MessageDetectCallback = std::function<void()>;

    Result InitializeDaemonMessageHandler();
    void ExitDaemonMessageHandler();
    void RegisterOnMessageDetect(MessageDetectCallback callback, dmi::MenuMessage desired_msg);

}