
#pragma once
#include <ul/menu/js/js_JavaScript.hpp>
#include <ul/menu/thm/thm_Theme.hpp>

namespace ul::menu::thm::api {

    namespace ui {

        // UI module

        void InitializeModule();

    }

    namespace os {

        // OS module

        void InitializeModule();

    }

    inline void InitializeJsApi() {
        js::Initialize();
        
        ui::InitializeModule();
        os::InitializeModule();
    }

    inline void FinalizeJsApi() {
        js::Finalize();
    }

    bool EvaluateScript(const MenuId id);

}