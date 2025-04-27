#include <ul/system/la/la_Open.hpp>
#include <ul/system/la/la_LibraryApplet.hpp>

namespace ul::system::la {

    // Thanks libnx for not exposing this stuff in a nice way :P
    // This is a rather extreme case of having to handle applets differently though (we are qlaunch after all)

    Result OpenPhotoViewerAllAlbumFilesForHomeMenu() {
        const struct {
            u8 album_arg;
        } in = { AlbumLaArg_ShowAllAlbumFilesForHomeMenu };
        return Start(AppletId_LibraryAppletPhotoViewer, 0x10000, &in, sizeof(in));
    }

    Result OpenWeb(const WebCommonConfig *cfg) {
        return Start(AppletId_LibraryAppletWeb, cfg->version, &cfg->arg, sizeof(cfg->arg));
    }

    Result OpenMiiEdit() {
        const auto mii_ver = hosversionAtLeast(10,2,0) ? 0x4 : 0x3;
        const MiiLaAppletInput in = {
            .version = mii_ver,
            .mode = MiiLaAppletMode_ShowMiiEdit,
            .special_key_code = MiiSpecialKeyCode_Normal
        };
        return Start(AppletId_LibraryAppletMiiEdit, -1, &in, sizeof(in));
    }

    Result OpenCabinet(const NfpLaStartParamTypeForAmiiboSettings type) {
        // Not sending any initial TagInfo/RegisterInfo makes the applet take care of the wait for the user to input amiibos
        // No neeed for us/uMenu to handle any amiibo functionality at all ;)
        
        const NfpLaStartParamForAmiiboSettings settings = {
            .unk_x0 = 0,
            .type = type,
            .flags = 1
        };
        return Start(AppletId_LibraryAppletCabinet, 1, &settings, sizeof(settings));
    }

    Result OpenMyPageMyProfile(const AccountUid uid) {
        if(hosversionAtLeast(9,0,0)) {
            const FriendsLaArg arg = {
                .hdr = {
                    .type = FriendsLaArgType_ShowMyProfile,
                    .uid = uid
                }
            };
            return Start(AppletId_LibraryAppletMyPage, 0x10000, &arg, sizeof(arg));
        }
        else {
            const FriendsLaArgV1 arg = {
                .hdr = {
                    .type = FriendsLaArgType_ShowMyProfile,
                    .uid = uid
                }
            };
            return Start(AppletId_LibraryAppletMyPage, 0x1, &arg, sizeof(arg));
        }
    }

    Result OpenPlayerSelectUserCreator() {
        PselUiSettings ui;
        UL_RC_TRY(pselUiCreate(&ui, PselUiMode_UserCreator));

        if(hosversionAtLeast(6,0,0)) {
            return Start(AppletId_LibraryAppletPlayerSelect, 0x20000, &ui, sizeof(ui));
        }
        else if (hosversionAtLeast(2,0,0)) {
            return Start(AppletId_LibraryAppletPlayerSelect, 0x10000, &ui, sizeof(ui));
        }
        else {
            return Start(AppletId_LibraryAppletPlayerSelect, 0x20000, &ui.settings, sizeof(ui.settings));
        }
    }

    namespace {

        // TODO (low priority): consider documenting this better, maybe a PR to libnx even

        enum class NetConnectType : u32 {
            Normal = 0,
            HomeMenu = 1,
            Starter = 2,
            UnableToConnectDialog = 3
        };

        struct NetConnectInput {
            NetConnectType type;
            u32 unk_32;
            Uuid unk_uuid;
            bool unk_flag;
            u8 pad[3];
        };
        static_assert(sizeof(NetConnectInput) == 0x1C);

    }

    Result OpenNetConnect() {
        const NetConnectType type = NetConnectType::HomeMenu;
        return Start(AppletId_LibraryAppletNetConnect, 1, &type, sizeof(type));
    }

    namespace {

        inline u32 GetControllerSupportArgSize() {
            if(hosversionBefore(8,0,0)) {
                return sizeof(HidLaControllerSupportArgV3);
            }
            else {
                return sizeof(HidLaControllerSupportArg);
            }
        }

        inline u32 GetControllerVersion() {
            if(hosversionAtLeast(11,0,0)) {
                return 0x8;
            }
            else if(hosversionAtLeast(8,0,0)) {
                return 0x7;
            }
            else if(hosversionAtLeast(6,0,0)) {
                return 0x5;
            }
            else if(hosversionAtLeast(3,0,0)) {
                return 0x4;
            }
            else {
                return 0x3;
            }
        }

    }

    Result OpenControllerKeyRemappingForSystem(const u32 npad_style_set, const HidNpadJoyHoldType hold_type) {
        const HidLaControllerSupportArgPrivate private_arg = {
            .private_size = sizeof(private_arg),
            .arg_size = GetControllerSupportArgSize(),
            .flag1 = 1,
            .mode = HidLaControllerSupportMode_ShowControllerKeyRemappingForSystem,
            .controller_support_caller = HidLaControllerSupportCaller_System,
            .npad_style_set = npad_style_set,
            .npad_joy_hold_type = hold_type
        };

        HidLaControllerKeyRemappingArg arg;
        hidLaCreateControllerKeyRemappingArg(&arg);

        return Start(AppletId_LibraryAppletController, GetControllerVersion(), &private_arg, sizeof(private_arg), &arg, sizeof(arg));
    }

}
