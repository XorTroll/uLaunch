#include <ul/system/system_Message.hpp>
#include <ul/ul_Result.hpp>
#include <ul/util/util_Scope.hpp>

namespace ul::system {

    Result PushSimpleSystemAppletMessage(const GeneralChannelMessage msg) {
        const auto msg_header = SystemAppletMessageHeader::Create(msg);

        AppletStorage st;
        UL_RC_TRY(appletCreateStorage(&st, sizeof(msg_header)));
        util::OnScopeExit close_st([&]() {
            appletStorageClose(&st);
        });

        UL_RC_TRY(appletStorageWrite(&st, 0, &msg_header, sizeof(msg_header)));
        UL_RC_TRY(appletPushToGeneralChannel(&st));
        return ResultSuccess;
    }

}
