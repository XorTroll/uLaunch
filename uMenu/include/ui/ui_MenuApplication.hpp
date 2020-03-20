
#pragma once
#include <ui/ui_StartupLayout.hpp>
#include <ui/ui_MenuLayout.hpp>
#include <ui/ui_ThemeMenuLayout.hpp>
#include <ui/ui_SettingsMenuLayout.hpp>
#include <ui/ui_LanguagesMenuLayout.hpp>
#include <am_DaemonMessages.hpp>

namespace ui
{
    enum class MenuType
    {
        Startup,
        Main,
        Settings,
        Theme,
        Languages,
    };

    class MenuApplication : public pu::ui::Application
    {
        public:
            using Application::Application;
            ~MenuApplication();
            PU_SMART_CTOR(MenuApplication)

            void OnLoad() override;

            void SetInformation(am::MenuStartMode mode, am::DaemonStatus status, JSON ui_json);
            void LoadMenu();
            void LoadStartupMenu();
            void LoadThemeMenu();
            void LoadSettingsMenu();
            void LoadSettingsLanguagesMenu();

            bool IsSuspended();
            bool IsTitleSuspended();
            bool IsHomebrewSuspended();
            bool EqualsSuspendedHomebrewPath(const std::string &path);
            u64 GetSuspendedApplicationId();
            void NotifyEndSuspended();
            bool LaunchFailed();
            void ShowNotification(const std::string &text, u64 timeout = 1500);

            template<typename T>
            inline T GetUIConfigValue(const std::string &name, T def)
            {
                return this->uijson.value<T>(name, def);
            }

            template<typename Elem>
            void ApplyConfigForElement(const std::string &menu, const std::string &name, std::shared_ptr<Elem> &Ref, bool also_visible = true)
            {
                if(this->uijson.count(menu))
                {
                    auto jmenu = this->uijson[menu];
                    if(jmenu.count(name))
                    {
                        auto jelem = jmenu[name];
                        bool set_coords = false;
                        if(also_visible)
                        {
                            bool visible = jelem.value("visible", true);
                            Ref->SetVisible(visible);
                            set_coords = visible;
                        }
                        else set_coords = true;
                        if(set_coords)
                        {
                            if(jelem.count("x"))
                            {
                                s32 x = jelem["x"];
                                Ref->SetX(x);
                            }
                            if(jelem.count("y"))
                            {
                                s32 y = jelem["y"];
                                Ref->SetY(y);
                            }
                        }
                    }
                }
            }

            void StartPlayBGM();
            void StopPlayBGM();

            StartupLayout::Ref &GetStartupLayout();
            MenuLayout::Ref &GetMenuLayout();
            ThemeMenuLayout::Ref &GetThemeMenuLayout();
            SettingsMenuLayout::Ref &GetSettingsMenuLayout();
            LanguagesMenuLayout::Ref &GetLanguagesMenuLayout();

            void SetSelectedUser(AccountUid user_id);
            AccountUid GetSelectedUser();
            MenuType GetCurrentLoadedMenu();
        private:
            am::MenuStartMode stmode;
            StartupLayout::Ref startupLayout;
            MenuLayout::Ref menuLayout;
            ThemeMenuLayout::Ref themeMenuLayout;
            SettingsMenuLayout::Ref settingsMenuLayout;
            LanguagesMenuLayout::Ref languagesMenuLayout;
            pu::ui::extras::Toast::Ref notifToast;
            am::DaemonStatus status;
            MenuType loaded_menu;
            JSON uijson;
            JSON bgmjson;
            bool bgm_loop;
            u32 bgm_fade_in_ms;
            u32 bgm_fade_out_ms;
            pu::audio::Music bgm;
    };

    void UiOnHomeButtonDetection();

    inline void RegisterHomeButtonDetection()
    {
        am::RegisterOnMessageDetect(&UiOnHomeButtonDetection, am::MenuMessage::HomeRequest);
    }
}