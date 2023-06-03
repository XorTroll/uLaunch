
#pragma once
#include <ul/smi/smi_Protocol.hpp>
#include <ul/system/smi/smi_Results.hpp>

namespace ul::system::smi {

    using namespace ul::smi;

    namespace impl {

        Result PopStorage(AppletStorage *st, const bool wait);
        Result PushStorage(AppletStorage *st);

    }

    using ScopedStorageReader = ul::smi::impl::ScopedStorageReaderBase<&impl::PopStorage>;
    using ScopedStorageWriter = ul::smi::impl::ScopedStorageWriterBase<&impl::PushStorage>;

    // System just receives commands from Menu

    inline Result ReceiveCommand(std::function<Result(const SystemMessage, ScopedStorageReader&)> pop_fn, std::function<Result(const SystemMessage, ScopedStorageWriter&)> push_fn) {
        return ul::smi::impl::ReceiveCommandImpl(pop_fn, push_fn);
    }

}