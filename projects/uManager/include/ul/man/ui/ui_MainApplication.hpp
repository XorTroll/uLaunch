
#pragma once
#include <ul/man/ui/ui_MainMenuLayout.hpp>
#include <ul/ul_Include.hpp>

namespace ul::man::ui {

    class MainApplication : public pu::ui::Application {
        private:
            MainMenuLayout::Ref main_menu_lyt;
            pu::ui::extras::Toast::Ref toast;
            bool ul_available;
            bool version_match;
            ul::Version ul_ver;

        public:
            using Application::Application;
            PU_SMART_CTOR(MainApplication)

            inline void Set(const bool ul_available, const bool version_match, const ul::Version ul_ver) {
                this->ul_available = ul_available;
                this->version_match = version_match;
                this->ul_ver = ul_ver;
            }
            
            inline bool IsAvailable() {
                return this->ul_available;
            }

            inline bool IsVersionMatch() {
                return this->version_match;
            }

            inline ul::Version GetVersion() {
                return this->ul_ver;
            }

            void OnLoad() override;

            void ShowNotification(const std::string &text);

            void OnInput(const u64 down, const u64 up, const u64 held);
    };

}
