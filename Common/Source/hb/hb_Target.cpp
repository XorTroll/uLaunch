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
        hb_hbl_Target(input.nro_path, input.argv, ctr);
    }
}