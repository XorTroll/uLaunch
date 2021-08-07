
#pragma once
#include <dmi/dmi_DaemonMenuInteraction.hpp>
#include <functional>

namespace am {

    using OnMessageCallback = std::function<void()>;

    Result InitializeDaemonMessageHandler();
    void ExitDaemonMessageHandler();
    void RegisterOnMessageDetect(OnMessageCallback callback, dmi::MenuMessage desired_msg);

}