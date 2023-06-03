#include <ul/system/smi/smi_SystemProtocol.hpp>
#include <ul/system/la/la_LibraryApplet.hpp>

namespace ul::system::smi {

    namespace impl {

        Result PopStorage(AppletStorage *st, const bool wait) {
            return ul::smi::impl::LoopWaitStorageFunctionImpl(&la::Pop, st, wait);
        }

        Result PushStorage(AppletStorage *st) {
            return ul::smi::impl::LoopWaitStorageFunctionImpl(&la::Push, st, false);
        }

    }

}