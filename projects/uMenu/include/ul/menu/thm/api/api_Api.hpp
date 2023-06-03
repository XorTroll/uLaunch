
#pragma once
#include <ul/menu/thm/api/api_JavaScript.hpp>
#include <pu/Plutonium>
#include <ul/fs/fs_Stdio.hpp>

namespace ul::menu::thm::api {

    inline void DebugLog(const std::string &msg) {
        const auto msg_log = msg + "\n";
        fs::WriteFileString("sdmc:/umad/debug.log", msg_log, false);
    }

    enum class EventType {
        Load,
        Input,
        HomePress
    };

    inline bool ConvertEventTypeToString(const EventType type, std::string &out_str) {
        #define _UL_MENU_THM_API_CONVERT_CASE(evt_type) case EventType::evt_type: { \
            out_str = #evt_type; \
            return true; \
        }

        switch(type) {
            _UL_MENU_THM_API_CONVERT_CASE(Load)
            _UL_MENU_THM_API_CONVERT_CASE(Input)
            _UL_MENU_THM_API_CONVERT_CASE(HomePress)
        }

        #undef _UL_MENU_THM_API_CONVERT_CASE

        return false;
    }

    inline bool ConvertStringToEventType(const std::string &str, EventType &out_type) {
        #define _UL_MENU_THM_API_CONVERT_CASE(evt_type) if(str == #evt_type) { \
            out_type = EventType::evt_type; \
            return true; \
        }

        _UL_MENU_THM_API_CONVERT_CASE(Load)
        _UL_MENU_THM_API_CONVERT_CASE(Input)
        _UL_MENU_THM_API_CONVERT_CASE(HomePress)

        #undef _UL_MENU_THM_API_CONVERT_CASE

        return false;
    }

    struct Element {
        js::GenericObject js_obj;
    };

    struct InnerElement {
        pu::ui::elm::Element::Ref elem;

        template<typename T>
        inline std::shared_ptr<T> GetAs() {
            return std::shared_ptr<T>(this->elem.get());
        }
    };

    struct MenuApiContext {
        std::map<std::string, Element> elems;
        std::map<EventType, js::FunctionObject> events;

        template<typename ...Ts>
        inline void OnEvent(EventType type, Ts ...ts) {
            auto it = this->events.find(type);
            if(it != this->events.end()) {
                it->second.Call<void>(ts...);
            }
        }

        inline void SetEvent(EventType type, js::FunctionObject fn) {
            auto it = this->events.find(type);
            if(it != this->events.end()) {
                it->second = fn;
            }
            else {
                this->events.emplace(type, fn);
            }
        }

    };

    void Initialize();

    inline void Finalize() {
        FinalizeJsApi();
    }

    bool LoadApi(const MenuId id, pu::ui::Layout::Ref menu_lyt);
    void UpdateCurrentMenu(const MenuId id);
    MenuId GetCurrentMenu();

    MenuApiContext &GetApiContext(const MenuId id);
    Element FindMenuElement(const MenuId id, const std::string &name);
    void AddMenuElement(const MenuId id, const std::string &name, Element elem);

    inline MenuApiContext &GetCurrentApiContext() {
        return GetApiContext(GetCurrentMenu());
    }

    inline Element FindCurrentMenuElement(const std::string &name) {
        return FindMenuElement(GetCurrentMenu(), name);
    }

    inline void AddCurrentMenuElement(const std::string &name, Element elem) {
        AddMenuElement(GetCurrentMenu(), name, elem);
    }

}