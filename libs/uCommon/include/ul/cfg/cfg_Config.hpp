
#pragma once
#include <switch.h>
#include <string>

namespace ul::cfg {

    enum class HomebrewLaunchMode : u32 {
        None = 0,
        Applet = 1,
        Application = 2
    };

    struct Config {
        static constexpr const char ConfigPath[] = "sdmc:/umad/config.json";

        u64 menu_takeover_program_id;
        u64 hb_applet_takeover_program_id;
        u64 hb_application_takeover_program_id;
        std::string active_theme_name;
        u32 entry_menu_h_count;
        HomebrewLaunchMode default_hb_launch_mode;

        bool Load();
        bool Save() const;

        inline bool ResetDefault() {
            // Take over eShop applet by default
            this->menu_takeover_program_id = 0x010000000000100B;
                
            // Take over parental-control applet by default
            this->hb_applet_takeover_program_id = 0x0100000000001001;
            
            // No donor application by default
            this->hb_application_takeover_program_id = 0;
            
            // Default theme name
            this->active_theme_name = "Default";

            // Default entry count
            this->entry_menu_h_count = 4;

            // No default launch mode
            this->default_hb_launch_mode = HomebrewLaunchMode::None;

            return this->Save();
        }

        inline bool EnsureLoad() {
            return this->Load() || this->ResetDefault();
        }
    };

}