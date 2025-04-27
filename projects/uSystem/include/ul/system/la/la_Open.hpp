
#pragma once
#include <ul/ul_Result.hpp>

namespace ul::system::la {

    Result OpenPhotoViewerAllAlbumFilesForHomeMenu();
    Result OpenWeb(const WebCommonConfig *cfg);
    Result OpenMiiEdit();
    Result OpenCabinet(const NfpLaStartParamTypeForAmiiboSettings type);
    Result OpenMyPageMyProfile(const AccountUid uid);
    Result OpenPlayerSelectUserCreator();
    Result OpenNetConnect();
    Result OpenControllerKeyRemappingForSystem(const u32 npad_style_set, const HidNpadJoyHoldType hold_type);

}
