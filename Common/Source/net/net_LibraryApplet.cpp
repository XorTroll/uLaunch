#include <net/net_LibraryApplet.hpp>

namespace net
{
    Result LaunchNetConnect()
    {
        u8 in[28] = {0};
        *(u32*)in = 1; // 0 = normal, 1 = qlaunch, 2 = starter?
        u8 out[8] = {0};
        LibAppletArgs args;
        libappletArgsCreate(&args, 0);
        libappletLaunch(AppletId_netConnect, &args, in, sizeof(in), out, sizeof(out), NULL);
        return *(u32*)out;
    }
}