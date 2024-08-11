
#pragma once
#include <ul/smi/smi_Protocol.hpp>
#include <functional>

namespace ul::menu::smi {

    using namespace ul::smi;

    using OnMessageCallback = std::function<void(const MenuMessageContext&)>;

    Result InitializeMenuMessageHandler();
    void FinalizeMenuMessageHandler();
    void RegisterOnMessageDetect(OnMessageCallback callback, const MenuMessage desired_msg = MenuMessage::Invalid);

}
