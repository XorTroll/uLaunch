#include <hb/hb_Target.hpp>
#include <am/am_QCommunications.hpp>

extern "C"
{
    u32 __nx_applet_type = AppletType_SystemApplication;
    u32 __nx_applet_exit_mode = 2;
}

hb::TargetInput hb_input;
bool target_once = false; // The application system allows classic hbl-ish system to exit to hbmenu

int main()
{
    hb_input = {};

    hb_input.nro_path[0] = '\0';
    hb_input.argv[0] = '\0';

    // Initialize applet, read stuff, close applet
    appletInitialize();
    hb::TargetInput ipt = {};
    auto rc = am::QApplicationReadStorage(&ipt, sizeof(ipt));
    if(R_SUCCEEDED(rc))
    {
        if(strlen(ipt.nro_path))
        {
            strcpy(hb_input.nro_path, ipt.nro_path);
            if(strlen(ipt.argv)) strcpy(hb_input.argv, ipt.argv);
        }
    }
    appletExit();

    if(hb_input.nro_path[0] == '\0') strcpy(hb_input.nro_path, "sdmc:/hbmenu.nro");
    if(hb_input.argv[0] == '\0') strcpy(hb_input.argv, hb_input.nro_path);

    hb::Target(hb_input, target_once);

    return 0;
}