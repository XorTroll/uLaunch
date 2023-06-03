#include <ul/menu/thm/api/api_Api.hpp>
#include <ul/menu/ui/ui_Application.hpp>

extern ul::menu::ui::Application::Ref g_Application;

namespace ul::menu::thm::api::ui {

    struct Color {
        pu::ui::Color clr;

        Color(u8 r, u8 g, u8 b, u8 a) {
            this->clr = { r, g, b, a };
        }
        UL_MENU_JS_CLASS_DECLARE(Color, u8, u8, u8, u8)

        u8 r_get() {
            return this->clr.r;
        }
        void r_set(u8 r) {
            this->clr.r = r;
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(r)

        u8 g_get() {
            return this->clr.g;
        }
        void g_set(u8 g) {
            this->clr.g = g;
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(g)

        u8 b_get() {
            return this->clr.b;
        }
        void b_set(u8 b) {
            this->clr.b = b;
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(b)
        
        u8 a_get() {
            return this->clr.a;
        }
        void a_set(u8 a) {
            this->clr.a = a;
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(a)

        UL_MENU_JS_CLASS_PROTOTYPE_START
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(r)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(g)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(b)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(a)
        UL_MENU_JS_CLASS_PROTOTYPE_END
    };
    
    struct TextBlock : public InnerElement {
        inline pu::ui::elm::TextBlock::Ref Get() {
            return this->GetAs<pu::ui::elm::TextBlock>();
        }
        
        TextBlock(s32 x, s32 y, std::string text) {
            this->elem = pu::ui::elm::TextBlock::New(x, y, text);
        }
        UL_MENU_JS_CLASS_DECLARE(TextBlock, s32, s32, std::string)

        void x_set(s32 x) {
            this->Get()->SetX(x);
        }
        s32 x_get() {
            return this->Get()->GetX();
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(x)

        void y_set(s32 y) {
            this->Get()->SetY(y);
        }
        s32 y_get() {
            return this->Get()->GetY();
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(y)

        u32 width_get() {
            return this->Get()->GetWidth();
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_G(width)

        u32 height_get() {
            return this->Get()->GetHeight();
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_G(height)

        void visible_set(bool visible) {
            this->Get()->SetVisible(visible);
        }
        bool visible_get() {
            return this->Get()->IsVisible();
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(visible)

        void horizontalAlign_set(u32 align) {
            this->Get()->SetHorizontalAlign(static_cast<pu::ui::elm::HorizontalAlign>(align));
        }
        u32 horizontalAlign_get() {
            return static_cast<u32>(this->Get()->GetHorizontalAlign());
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(horizontalAlign)

        void verticalAlign_set(u32 align) {
            this->Get()->SetVerticalAlign(static_cast<pu::ui::elm::VerticalAlign>(align));
        }
        u32 verticalAlign_get() {
            return static_cast<u32>(this->Get()->GetVerticalAlign());
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(verticalAlign)

        void text_set(std::string text) {
            this->Get()->SetText(text);
        }
        std::string text_get() {
            return this->Get()->GetText();
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(text)

        void font_set(std::string font) {
            this->Get()->SetFont(font);
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_S(font)

        void color_set(js::Object<Color> color) {
            this->Get()->SetColor(color->clr);
        }
        js::Object<Color> color_get() {
            const auto clr = this->Get()->GetColor();
            return js::Object<Color>(clr.r, clr.g, clr.b, clr.a);
        }
        UL_MENU_JS_CLASS_DECLARE_PROPERTY_GS(color)

        UL_MENU_JS_CLASS_PROTOTYPE_START
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(x)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(y)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_G(width)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_G(height)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(visible)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(horizontalAlign)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(verticalAlign)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(text)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_S(font)
        UL_MENU_JS_CLASS_PROTOTYPE_EXPORT_PROPERTY_GS(color)
        UL_MENU_JS_CLASS_PROTOTYPE_END
    };

    void loadFont(std::string font_name, u32 font_size, std::string ttf_path) {
        pu::ui::render::AddFontFile(font_name, font_size, ttf_path);
    }
    UL_MENU_JS_DEFINE_FUNCTION(loadFont)

    js::GenericObject findElement(std::string elem_name) {
        const auto elem = FindCurrentMenuElement(elem_name);
        return elem.js_obj;
    }
    UL_MENU_JS_DEFINE_FUNCTION(findElement)

    js::GenericObject findMenuElement(std::string menu_name, std::string elem_name) {
        MenuId menu_id;
        if(ConvertStringToMenuId(menu_name, menu_id)) {
            auto elem = FindMenuElement(menu_id, elem_name);
            return elem.js_obj;
        }
        return js::GenericObject::Null();
    }
    UL_MENU_JS_DEFINE_FUNCTION(findMenuElement)

    void addElement(std::string elem_name, js::GenericObject elem) {
        AddCurrentMenuElement(elem_name, { elem });
    }

    void addMenuElement(std::string menu_name, std::string elem_name, js::GenericObject elem) {
        MenuId menu_id;
        if(ConvertStringToMenuId(menu_name, menu_id)) {
            AddMenuElement(menu_id, elem_name, { elem });
        }
    }

    std::string getCurrentMenu() {
        std::string menu_name;
        if(ConvertMenuIdToString(GetCurrentMenu(), menu_name)) {
            return menu_name;
        }
        return "";
    }
    UL_MENU_JS_DEFINE_FUNCTION(getCurrentMenu)

    void render() {
        g_Application->CallForRender();
    }
    UL_MENU_JS_DEFINE_FUNCTION(render)

    void showNotification(std::string msg, u64 timeout_ms) {
        g_Application->ShowNotification(msg, timeout_ms);
    }
    UL_MENU_JS_DEFINE_FUNCTION(showNotification)

    void setEvent(std::string evt_name, js::FunctionObject fn) {
        EventType evt_type;
        if(ConvertStringToEventType(evt_name, evt_type)) {
            auto &api_ctx = GetCurrentApiContext();
            api_ctx.SetEvent(evt_type, fn);
        }
    }
    UL_MENU_JS_DEFINE_FUNCTION(setEvent)

    void setMenuEvent(std::string menu_name, std::string evt_name, js::FunctionObject fn) {
        MenuId menu_id;
        if(ConvertStringToMenuId(menu_name, menu_id)) {
            EventType evt_type;
            if(ConvertStringToEventType(evt_name, evt_type)) {
                auto &api_ctx = GetApiContext(menu_id);
                api_ctx.SetEvent(evt_type, fn);
            }
        }
    }
    UL_MENU_JS_DEFINE_FUNCTION(setMenuEvent)

    UL_MENU_JS_MODULE_DEFINE_EXPORTS_START(ui)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_CLASS(Color)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_CLASS(TextBlock)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(loadFont)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(findElement)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(findMenuElement)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(getCurrentMenu)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(render)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(showNotification)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(setEvent)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_ADD_FUNCTION(setMenuEvent)
    UL_MENU_JS_MODULE_DEFINE_EXPORTS_END

    void InitializeModule() {
        UL_MENU_JS_IMPORT_MODULE(ui)
    }

}