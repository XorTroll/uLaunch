#include <hb/hb_Target.hpp>

extern "C"
{
    void hb_hbl_Target(const char *path, const char *argv, int counter);
}

namespace hb
{
    void Target(TargetInput input, bool once)
    {
        int ctr = once ? 1 : -1;
        // If we're loading hbmenu, it makes no sense to load the NRO once (opening a homebrew would terminate it, what literally goes against hbmenu's main functionality)
        if(strcmp("sdmc:/hbmenu.nro", input.nro_path) == 0) ctr = -1;
        hb_hbl_Target(input.nro_path, input.argv, ctr);
    }
}