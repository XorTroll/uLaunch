
#pragma once
#include <ui/ui_StartupLayout.hpp>
#include <ui/ui_MenuLayout.hpp>
#include <ui/ui_ThemeMenuLayout.hpp>
#include <ui/ui_SettingsMenuLayout.hpp>

namespace ui
{
    class QMenuApplication : public pu::ui::Application
    {
        public:
            using Application::Application;
            ~QMenuApplication();
            PU_SMART_CTOR(QMenuApplication)

            void OnLoad() override;

            void SetStartMode(am::QMenuStartMode mode);
            void LoadMenu();
            void LoadStartupMenu();
            void LoadThemeMenu();
            void LoadSettingsMenu();

            bool IsSuspended();
            bool IsTitleSuspended();
            bool IsHomebrewSuspended();
            std::string GetSuspendedHomebrewPath();
            u64 GetSuspendedApplicationId();
            void NotifyEndSuspended();
            bool LaunchFailed();
            void ShowNotification(std::string text, u64 timeout = 1500);
            bool IsFadeReady();

            template<typename T>
            T GetUIConfigValue(std::string name, T def)
            {
                return this->uijson.value<T>(name, def);
            }

            template<typename Elem>
            void ApplyConfigForElement(std::string menu, std::string name, std::shared_ptr<Elem> &Ref, bool also_visible = true)
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

            void SetSelectedUser(u128 user_id);
            u128 GetSelectedUser();
        private:
            am::QMenuStartMode stmode;
            StartupLayout::Ref startupLayout;
            MenuLayout::Ref menuLayout;
            ThemeMenuLayout::Ref themeMenuLayout;
            SettingsMenuLayout::Ref settingsMenuLayout;
            pu::ui::extras::Toast::Ref notifToast;
            am::QSuspendedInfo suspinfo;
            u128 selected_user;
            JSON uijson;
            JSON bgmjson;
            bool bgm_loop;
            u32 bgm_fade_in_ms;
            u32 bgm_fade_out_ms;
            pu::audio::Music bgm;
    };
}