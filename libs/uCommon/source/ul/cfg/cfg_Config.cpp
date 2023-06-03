#include <ul/cfg/cfg_Config.hpp>
#include <extras/json.hpp>
#include <ul/fs/fs_Stdio.hpp>
#include <ul/util/util_String.hpp>

namespace ul::cfg {

    bool Config::Load() {
        try {
            std::string config_json_str;
            if(!fs::ReadFileString(ConfigPath, config_json_str)) {
                return false;
            }

            const auto config_json = nlohmann::json::parse(config_json_str);

            #define _UL_CFG_CONFIG_LOAD_PROGRAM_ID(name, required) ({ \
                const auto raw_program_id = config_json.value(#name, ""); \
                this->name = strtoull(raw_program_id.c_str(), nullptr, 16); \
                if(required && (this->name == 0)) { \
                    return false; \
                } \
            })

            #define _UL_CFG_CONFIG_LOAD_REQUIRED_STRING(name) ({ \
                this->name = config_json.value(#name, ""); \
                if(this->name.empty()) { \
                    return false; \
                } \
            })

            #define _UL_CFG_CONFIG_LOAD_VALUE(name, required) ({ \
                if(config_json.count(#name)) { \
                    this->name = config_json[#name].get<decltype(this->name)>(); \
                } \
                else if(required) { \
                    return false; \
                } \
            })

            #define _UL_CFG_CONFIG_LOAD_CAST_VALUE(name, tmp_type, required) ({ \
                if(config_json.count(#name)) { \
                    this->name = static_cast<decltype(this->name)>(config_json[#name].get<tmp_type>()); \
                } \
                else if(required) { \
                    return false; \
                } \
            })

            _UL_CFG_CONFIG_LOAD_PROGRAM_ID(menu_takeover_program_id, true);
            _UL_CFG_CONFIG_LOAD_PROGRAM_ID(hb_applet_takeover_program_id, true);
            _UL_CFG_CONFIG_LOAD_PROGRAM_ID(hb_application_takeover_program_id, false);
            _UL_CFG_CONFIG_LOAD_VALUE(active_theme_name, true);
            _UL_CFG_CONFIG_LOAD_VALUE(entry_menu_h_count, true);
            _UL_CFG_CONFIG_LOAD_CAST_VALUE(default_hb_launch_mode, u32, true);

            #undef _UL_CFG_CONFIG_LOAD_CAST_VALUE
            #undef _UL_CFG_CONFIG_LOAD_VALUE
            #undef _UL_CFG_CONFIG_LOAD_REQUIRED_STRING
            #undef _UL_CFG_CONFIG_LOAD_REQUIRED_PROGRAM_ID

            return true;
        }
        catch(std::exception&) {
            return false;
        }
    }

    bool Config::Save() const {
        auto config_json = nlohmann::json::object();

        #define _UL_CFG_CONFIG_STORE_PROGRAM_ID(name) ({ \
            if(this->name != 0) { \
                config_json[#name] = util::FormatProgramId(this->name); \
            } \
        })
        
        #define _UL_CFG_CONFIG_STORE_VALUE(name) ({ \
            config_json[#name] = this->name; \
        })

        #define _UL_CFG_CONFIG_STORE_CAST_VALUE(name, tmp_type) ({ \
            config_json[#name] = static_cast<tmp_type>(this->name); \
        })

        _UL_CFG_CONFIG_STORE_PROGRAM_ID(menu_takeover_program_id);
        _UL_CFG_CONFIG_STORE_PROGRAM_ID(hb_applet_takeover_program_id);
        _UL_CFG_CONFIG_STORE_PROGRAM_ID(hb_application_takeover_program_id);
        _UL_CFG_CONFIG_STORE_VALUE(active_theme_name);
        _UL_CFG_CONFIG_STORE_VALUE(entry_menu_h_count);
        _UL_CFG_CONFIG_STORE_CAST_VALUE(default_hb_launch_mode, u32);

        #undef _UL_CFG_CONFIG_STORE_CAST_VALUE
        #undef _UL_CFG_CONFIG_STORE_VALUE
        #undef _UL_CFG_CONFIG_STORE_STRING
        #undef _UL_CFG_CONFIG_STORE_PROGRAM_ID

        const auto config_json_str = config_json.dump(4);
        return fs::WriteFile(ConfigPath, config_json_str.c_str(), config_json_str.length(), true);
    }

}