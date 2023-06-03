
#pragma once
#include <ul/menu/smi/smi_SystemMessageHandler.hpp>
#include <ul/menu/ui/menu/menu_MainMenuLayout.hpp>
#include <ul/menu/ui/menu/menu_SettingsMenuLayout.hpp>

namespace ul::menu::ui {

    void UiOnHomeButtonDetection();
    void UiOnSdCardEjectedDetection();

    class Application : public pu::ui::Application {
        private:
            menu::MainMenuLayout::Ref main_menu_lyt;
            menu::SettingsMenuLayout::Ref settings_menu_lyt;
            pu::ui::extras::Toast::Ref notif_toast;

        public:
            using pu::ui::Application::Application;
            ~Application() {}
            PU_SMART_CTOR(Application)

            static inline void RegisterHomeButtonDetection() {
                smi::RegisterOnMessageDetect(&UiOnHomeButtonDetection, smi::MenuMessage::HomeRequest);
            }

            static inline void RegisterSdCardEjectedDetection() {
                smi::RegisterOnMessageDetect(&UiOnSdCardEjectedDetection, smi::MenuMessage::SdCardEjected);
            }

            inline void LoadMainMenu() {
                this->LoadLayout(this->main_menu_lyt);
            }

            inline void LoadSettingsMenu() {
                this->LoadLayout(this->settings_menu_lyt);
            }

            void OnLoad() override;

            void ShowNotification(const std::string &text, const u64 timeout = 1500);
    };

}