#include <ul/menu/thm/api/api_Api.hpp>
#include <ul/menu/ui/ui_Application.hpp>

extern ul::menu::ui::Application::Ref g_Application;

namespace ul::menu::thm::api {

    namespace {

        std::vector<std::pair<MenuId, MenuApiContext>> g_ApiContextTable;

        MenuId g_CurrentMenuId;

        constexpr MenuId MenuIdList[] = {
            MenuId::MainMenu
        };
        constexpr size_t MenuIdCount = sizeof(MenuIdList) / sizeof(MenuId);

    }

    void Initialize() {
        InitializeJsApi();
        for(size_t i = 0; i < MenuIdCount; i++) {
            g_ApiContextTable.push_back({ MenuIdList[i], {} });
        }
    }

    bool LoadApi(const MenuId id, pu::ui::Layout::Ref menu_lyt) {
        api::DebugLog("Pre eval js");
        if(!EvaluateScript(id)) {
            return false;
        }
        api::DebugLog("Post eval js");

        return true;
    }

    void UpdateCurrentMenu(const MenuId id) {
        g_CurrentMenuId = id;
    }

    MenuId GetCurrentMenu() {
        return g_CurrentMenuId;
    }

    MenuApiContext &GetApiContext(const MenuId id) {
        for(auto &[c_id, api_ctx] : g_ApiContextTable) {
            if(c_id == id) {
                return api_ctx;
            }
        }

        fatalThrow(0xbeefbabe);
        __builtin_unreachable();
    }

    Element FindMenuElement(const MenuId id, const std::string &name) {
        auto &api_ctx = GetApiContext(id);
        for(const auto &[elem_name, elem] : api_ctx.elems) {
            if(name == elem_name) {
                return elem;
            }
        }
        return {};
    }

    void AddMenuElement(const MenuId id, const std::string &name, Element elem) {
        auto &api_ctx = GetApiContext(id);
        auto inner_elm = elem.js_obj.Get<InnerElement>();
        g_Application->GetLayout<ul::menu::ui::MenuLayout>()->Add(inner_elm->elem);
        api_ctx.elems.emplace(name, elem);
    }

}