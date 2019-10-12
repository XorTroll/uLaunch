
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

            bool IsSuspended();
            bool IsTitleSuspended();
            bool IsHomebrewSuspended();
            std::string GetSuspendedHomebrewPath();
            u64 GetSuspendedApplicationId();
            bool LaunchFailed();

            void SetSelectedUser(u128 user_id);
            u128 GetSelectedUser();
        private:
            am::QMenuStartMode stmode;
            StartupLayout::Ref startupLayout;
            MenuLayout::Ref menuLayout;
            am::QSuspendedInfo suspinfo;
            u128 selected_user;
    };
}