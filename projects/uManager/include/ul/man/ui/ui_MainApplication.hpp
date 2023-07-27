
#pragma once
#include <ul/man/ui/ui_MainMenuLayout.hpp>

namespace ul::man::ui {

    class MainApplication : public pu::ui::Application {
        private:
            MainMenuLayout::Ref main_menu_lyt;
            pu::ui::extras::Toast::Ref toast;

        public:
            using Application::Application;
            PU_SMART_CTOR(MainApplication)

            void OnLoad() override;

            void ShowNotification(const std::string &text);

            void OnInput(const u64 down, const u64 up, const u64 held);
    };

}