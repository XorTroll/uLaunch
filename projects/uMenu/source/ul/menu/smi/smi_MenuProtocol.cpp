#include <ul/menu/smi/smi_MenuProtocol.hpp>

namespace ul::menu::smi {

    namespace impl {

        Result PopStorage(AppletStorage *st, const bool wait) {
            return ul::smi::impl::LoopWaitStorageFunctionImpl(&appletPopInData, st, wait);
        }

        Result PushStorage(AppletStorage *st) {
            return ul::smi::impl::LoopWaitStorageFunctionImpl(&appletPushOutData, st, false);
        }

    }

}