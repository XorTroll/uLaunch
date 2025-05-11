
#pragma once
#include <ul/smi/smi_Protocol.hpp>

namespace ul::menu::smi {

    using namespace ul::smi;

    namespace impl {

        Result PopStorage(AppletStorage *st, const bool wait);
        Result PushStorage(AppletStorage *st);

    }

    using ScopedStorageReader = ul::smi::impl::ScopedStorageReaderBase<&impl::PopStorage>;
    using ScopedStorageWriter = ul::smi::impl::ScopedStorageWriterBase<&impl::PushStorage>;

    // Menu just sends commands to System

    inline Result SendCommand(const SystemMessage msg, std::function<Result(ScopedStorageWriter&)> push_fn, std::function<Result(ScopedStorageReader&)> pop_fn) {
        return ul::smi::impl::SendCommandImpl(msg, push_fn, pop_fn);
    }

}
