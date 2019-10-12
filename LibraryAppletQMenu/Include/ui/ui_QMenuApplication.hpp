
#pragma once
#include <ui/ui_StartupLayout.hpp>
#include <ui/ui_MenuLayout.hpp>

namespace ui
{
    class QMenuApplication : public pu::ui::Application
    {
        public:
            using Application::Application;
            PU_SMART_CTOR(QMenuApplication)

            void OnLoad() override;

            void SetStartMode(am::QMenuStartMode mode);
            void LoadMenu();

            bool IsTitleSuspended();
            bool LaunchFailed();
            void SetTitleSuspended(bool suspended);
            void SetSelectedUser(u128 user_id);
            u128 GetSelectedUser();
            void SetSuspendedApplicationId(u64 app_id);
            u64 GetSuspendedApplicationId();
        private:
            am::QMenuStartMode stmode;
            StartupLayout::Ref startupLayout;
            MenuLayout::Ref menuLayout;

            bool tsuspended;
            u128 selected_user;
            u64 suspended_appid;
    };
}