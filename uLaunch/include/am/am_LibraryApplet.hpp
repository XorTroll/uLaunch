
#pragma once
#include <ul_Include.hpp>
#include <map>

namespace am
{
    namespace controller
    {
        struct InitialArg
        {
            u32 this_size;
            u32 arg_size;
            bool unk2; // unk2 (and unk3?) need to be true for controller to launch
            bool unk3;
            u8 mode; // Changing this doesn't seem to affect, but this is the mode according to SDK RE...?
            u32 npad_style_set;
            u32 npad_joy_hold_type;
        } PACKED;

        struct MainArg
        {
            s8 min_player_count;
            s8 max_player_count;
            bool take_over_connection;
            bool left_justify;
            bool permit_dual_joy;
            bool single_mode;
            bool use_colors;
            u32 colors[4];
            bool use_controller_names;
            char controller_names[4][0x81];
        } PACKED;
    }

    bool LibraryAppletIsActive();
    void LibraryAppletSetMenuAppletId(AppletId id);
    bool LibraryAppletIsMenu();
    void LibraryAppletTerminate();
    Result LibraryAppletStart(AppletId id, u32 la_version, void *in_data, size_t in_size);
    Result LibraryAppletSend(void *data, size_t size);
    Result LibraryAppletRead(void *data, size_t size);
    Result WebAppletStart(WebCommonConfig *web);

    u64 LibraryAppletGetProgramIdForAppletId(AppletId id);
    AppletId LibraryAppletGetAppletIdForProgramId(u64 id);

    AppletId LibraryAppletGetId();

    static constexpr AppletId InvalidAppletId = (AppletId)0;
}