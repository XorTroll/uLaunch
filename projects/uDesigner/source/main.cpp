#include <ul/design/design_Include.hpp>
#include <sstream>

EM_JS(int, GetCanvasWidth, (), {
  return Module.canvas.width;
});

EM_JS(int, GetCanvasHeight, (), {
  return Module.canvas.height;
});

EM_JS(void, ResizeCanvas, (), {
    js_ResizeCanvas();
});

EM_JS(void, OpenUrl, (const char *url), {
    window.open(UTF8ToString(url), "_blank");
});

// Note: defined in JS since it is also used from there
EM_JS(void, ShowError, (const char *error), {
    js_ShowError(UTF8ToString(error));
});

EM_JS(void, ShowInformation, (const char *info), {
    alert(UTF8ToString(info));
});

namespace {

    constexpr auto ClearColor = ImVec4(0.14, 0.14, 0.4, 1.0);

    GLFWwindow *g_Window;
    int g_Width;
    int g_Height;

    bool g_DisplayAboutWindow = false;

    inline ImColor ParseHexColor(const std::string &color) {
        if(color.length() >= 7) {
            std::string r = "00";
            std::string g = "00";
            std::string b = "00";
            std::string a = "FF";

            if(color.length() >= 9) {
                a = color.substr(7, 2);
            }
            if(color.length() >= 7) {
                r = color.substr(1, 2);
                g = color.substr(3, 2);
                b = color.substr(5, 2);
            }
            
            const auto r_val = static_cast<u8>(std::stoul(r, nullptr, 16));
            const auto g_val = static_cast<u8>(std::stoul(g, nullptr, 16));
            const auto b_val = static_cast<u8>(std::stoul(b, nullptr, 16));
            const auto a_val = static_cast<u8>(std::stoul(a, nullptr, 16));
            return ImColor(r_val, g_val, b_val, a_val);
        }
        else {
            emscripten_log(EM_LOG_CONSOLE, "Invalid color '%s' (expected '#RRGGBBAA' format)", color.c_str());
            return 0;
        }
    }

    inline std::string GenerateHexColor(const ImColor color) {
        std::stringstream strm;
        strm << "#";
        strm << std::setw(2) << std::setfill('0') << std::hex << (u32)(color.Value.x * 255.0f);
        strm << std::setw(2) << std::setfill('0') << std::hex << (u32)(color.Value.y * 255.0f);
        strm << std::setw(2) << std::setfill('0') << std::hex << (u32)(color.Value.z * 255.0f);
        strm << std::setw(2) << std::setfill('0') << std::hex << (u32)(color.Value.w * 255.0f);

        return strm.str();
    }

}

namespace {

    void *g_ThemeZipData = nullptr;
    size_t g_ThemeZipDataSize = 0;
    zip_t *g_ThemeZip = nullptr;

    void *g_FontData = nullptr;
    size_t g_FontDataSize = 0;

    nlohmann::json g_Manifest;
    char g_ManifestName[256] = {};
    char g_ManifestDescription[256] = {};
    char g_ManifestRelease[256] = {};
    char g_ManifestAuthor[256] = {};

    inline bool IsLoadedTheme() {
        return g_ThemeZip != nullptr;
    }

    #define IMGUI_COLOR_TO_U32(color) IM_COL32((u32)(color.Value.x * 255.0f), (u32)(color.Value.y * 255.0f), (u32)(color.Value.z * 255.0f), (u32)(color.Value.w * 255.0f))

    nlohmann::json g_UiSettings;
    int g_suspended_app_final_alpha;
    ImColor g_text_color;
    ImColor g_menu_focus_color;
    ImColor g_menu_bg_color;
    ImColor g_dialog_title_color;
    ImColor g_dialog_cnt_color;
    ImColor g_dialog_opt_color;
    ImColor g_dialog_color;
    ImColor g_dialog_over_color;

    nlohmann::json g_SoundSettings;
    bool g_SoundSettingsLoaded = false;
    std::optional<bool> g_bgm_loop;
    std::optional<int> g_bgm_fade_in_ms;
    std::optional<int> g_bgm_fade_out_ms;
    std::optional<bool> g_main_bgm_loop;
    std::optional<int> g_main_bgm_fade_in_ms;
    std::optional<int> g_main_bgm_fade_out_ms;
    std::optional<bool> g_startup_bgm_loop;
    std::optional<int> g_startup_bgm_fade_in_ms;
    std::optional<int> g_startup_bgm_fade_out_ms;
    std::optional<bool> g_themes_bgm_loop;
    std::optional<int> g_themes_bgm_fade_in_ms;
    std::optional<int> g_themes_bgm_fade_out_ms;
    std::optional<bool> g_settings_bgm_loop;
    std::optional<int> g_settings_bgm_fade_in_ms;
    std::optional<int> g_settings_bgm_fade_out_ms;

    constexpr u32 WindowTitleBarExtraHeight = 20;
    float g_Scale = 0.6f;

    ImFont *g_FontStandard_Small;
    ImFont *g_FontStandard_Medium;
    ImFont *g_FontStandard_MediumLarge;
    ImFont *g_FontStandard_Large;
    ImFont *g_FontExtended_Small;
    ImFont *g_FontExtended_Medium;
    ImFont *g_FontExtended_MediumLarge;
    ImFont *g_FontExtended_Large;

}

bool LoadThemeJsonAsset(const char *path, nlohmann::json &out_json) {
    auto zip_rc = zip_entry_open(g_ThemeZip, path);
    if(zip_rc != 0) {
        emscripten_log(EM_LOG_CONSOLE, "Unable to open theme ZIP JSON asset at '%s'...: %d", path, zip_rc);
        return false;
    }

    void *json_data;
    size_t json_data_size;
    zip_rc = zip_entry_read(g_ThemeZip, &json_data, &json_data_size);
    if(zip_rc <= 0) {
        zip_entry_close(g_ThemeZip);
        emscripten_log(EM_LOG_CONSOLE, "Unable to read theme ZIP JSON asset at '%s'...: %d", path, zip_rc);
        return false;
    }

    std::string json_str(reinterpret_cast<const char*>(json_data), json_data_size);
    free(json_data);
    zip_entry_close(g_ThemeZip);

    out_json = nlohmann::json::parse(json_str);
    return true;
}

bool SaveThemeAsset(zip_t *theme_zip, const std::string &path, const void *data, const size_t data_size) {
    auto zip_rc = zip_entry_open(theme_zip, path.c_str());
    if(zip_rc != 0) {
        emscripten_log(EM_LOG_CONSOLE, "Unable to open theme ZIP asset at '%s'...", path.c_str());
        zip_close(theme_zip);
        return false;
    }

    zip_rc = zip_entry_write(theme_zip, data, data_size);
    if(zip_rc != 0) {
        zip_entry_close(theme_zip);
        emscripten_log(EM_LOG_CONSOLE, "Unable to write theme ZIP asset at '%s': %d", path.c_str(), zip_rc);
        zip_close(theme_zip);
        return false;
    }

    zip_entry_close(theme_zip);
    return true;
}

inline bool SaveThemeJsonAsset(zip_t *theme_zip, const std::string &path, const nlohmann::json &json) {
    const auto json_str = json.dump(4);
    return SaveThemeAsset(theme_zip, path, json_str.c_str(), json_str.length());
}

constexpr const char *AllowedImageFormats[] = {
    "png",
    "jpg",
    "jpeg"
};

struct TextElement {
    std::string text;
    int font_size;
};

void DrawRectangle(const u32 x, const u32 y, const u32 width, const u32 height, const ImU32 color) {
    auto draw_list = ImGui::GetWindowDrawList();
    const auto canvas_pos = ImGui::GetCursorScreenPos();

    const auto pos_a = ImVec2(canvas_pos.x + x * g_Scale, canvas_pos.y + y * g_Scale);
    const auto pos_b = ImVec2(pos_a.x + width * g_Scale, pos_a.y + height * g_Scale);
    draw_list->AddRectFilled(pos_a, pos_b, color);
}

void ProcessText(const std::string &text, const ul::design::FontSize font_size, u32 &out_width, u32 &out_height, ImFont *&out_font, float &out_font_size) {
    switch(static_cast<ul::design::FontSize>(font_size)) {
        case ul::design::FontSize::Small:
            out_font = g_FontStandard_Small;
            out_font_size = ul::design::FontSizeSmall;
            break;
        case ul::design::FontSize::Medium:
            out_font = g_FontStandard_Medium;
            out_font_size = ul::design::FontSizeMedium;
            break;
        case ul::design::FontSize::MediumLarge:
            out_font = g_FontStandard_MediumLarge;
            out_font_size = ul::design::FontSizeMediumLarge;
            break;
        case ul::design::FontSize::Large:
            out_font = g_FontStandard_Large;
            out_font_size = ul::design::FontSizeLarge;
            break;
        default:
            break;
    }

    ImGui::PushFont(out_font);
    const auto text_size = ImGui::CalcTextSize(text.c_str());
    ImGui::PopFont();

    out_width = (u32)text_size.x;
    out_height = (u32)text_size.y;
}

void DrawTextAt(const std::string &text, const ul::design::FontSize font_size, const u32 x, const u32 y) {
    auto draw_list = ImGui::GetWindowDrawList();
    const auto canvas_pos = ImGui::GetCursorScreenPos();

    ImFont *draw_font;
    float draw_font_size;
    switch(font_size) {
        case ul::design::FontSize::Small:
            draw_font = g_FontStandard_Small;
            draw_font_size = ul::design::FontSizeSmall;
            break;
        case ul::design::FontSize::Medium:
            draw_font = g_FontStandard_Medium;
            draw_font_size = ul::design::FontSizeMedium;
            break;
        case ul::design::FontSize::MediumLarge:
            draw_font = g_FontStandard_MediumLarge;
            draw_font_size = ul::design::FontSizeMediumLarge;
            break;
        case ul::design::FontSize::Large:
            draw_font = g_FontStandard_Large;
            draw_font_size = ul::design::FontSizeLarge;
            break;
        default:
            break;
    }

    const auto pos = ImVec2(canvas_pos.x + x * g_Scale, canvas_pos.y + y * g_Scale);
    draw_list->AddText(draw_font, draw_font_size * g_Scale, pos, IMGUI_COLOR_TO_U32(g_text_color), text.c_str());
}

struct ImageElement {
    int width;
    int height;
    GLuint gl_tex;

    constexpr ImageElement() : width(0), height(0), gl_tex(0) {}

    inline bool IsValid() const {
        return (this->gl_tex != 0) && (this->width > 0) && (this->height > 0);
    }

    inline void Dispose() {
        if(this->IsValid()) {
            glDeleteTextures(1, &this->gl_tex);
            this->gl_tex = 0;
            this->width = 0;
            this->height = 0;
        }
    }

    inline void LoadFrom(void *img_data, const size_t img_data_size, const u32 custom_width, const u32 custom_height) {
        int data_width;
        int data_height;
        int bpp;
        auto stb_data = stbi_load_from_memory(reinterpret_cast<const u8*>(img_data), img_data_size, &data_width, &data_height, &bpp, 0);

        GLuint gl_tex;
        glGenTextures(1, &gl_tex);
        glBindTexture(GL_TEXTURE_2D, gl_tex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if(bpp == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data_width, data_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, stb_data);
        }
        else if(bpp == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, data_width, data_height, 0, GL_RGB, GL_UNSIGNED_BYTE, stb_data);
        }
        else {
            emscripten_log(EM_LOG_CONSOLE, "Unexpected bpp amount in image: %d", bpp);
        }

        stbi_image_free(stb_data);
        free(img_data);

        this->width = (custom_width > 0) ? custom_width : data_width;
        this->height = (custom_height > 0) ? custom_height : data_height;
        this->gl_tex = gl_tex;
    }
};

struct ElementLoadContext {
    std::string img_path;
    void *img_raw_data;
    size_t img_raw_data_size;
    u32 img_custom_width;
    u32 img_custom_height;

    std::string text_text;

    inline static ElementLoadContext ForAssetImage(const std::string &path, const u32 custom_width = 0, const u32 custom_height = 0) {
        return {
            .img_path = path,
            .img_custom_width = custom_width,
            .img_custom_height = custom_height
        };
    }

    inline static ElementLoadContext ForRawImage(void *raw_data, size_t raw_data_size, const u32 custom_width = 0, const u32 custom_height = 0) {
        return {
            .img_raw_data = raw_data,
            .img_raw_data_size = raw_data_size,
            .img_custom_width = custom_width,
            .img_custom_height = custom_height
        };
    }

    inline static ElementLoadContext ForText(const std::string &text) {
        return {
            .text_text = text
        };
    }
};

struct Element {
    ul::design::MenuType menu;
    ul::design::ElementType type;

    std::string ui_name;
    ElementLoadContext load_ctx;

    int id;
    int x;
    int y;
    int h_align;
    int v_align;

    ImageElement el_img;
    TextElement el_text;

    inline Element(const ul::design::MenuType menu, const ul::design::ElementType type, const std::string &ui_name, const ElementLoadContext load_ctx) : menu(menu), type(type), ui_name(ui_name), load_ctx(load_ctx), id(0), x(0), y(0), h_align(static_cast<int>(ul::design::HorizontalAlign::Left)), v_align(static_cast<int>(ul::design::VerticalAlign::Top)) {}

    inline nlohmann::json GetSettings() {
        nlohmann::json el_json;
        switch(this->menu) {
            case ul::design::MenuType::None: {
                if(!g_UiSettings.count(this->ui_name)) {
                    emscripten_log(EM_LOG_CONSOLE, "No UI settings found for element '%s'...", this->ui_name.c_str());
                    return {};
                }
                
                el_json = g_UiSettings[this->ui_name];
                break;
            }

            #define _MENU_OPTION(type) \
                case ul::design::MenuType::type: { \
                    constexpr auto name = ul::design::MenuSettingsNames[static_cast<u32>(ul::design::MenuType::type)]; \
                    if(!g_UiSettings.count(name)) { \
                        emscripten_log(EM_LOG_CONSOLE, "No UI settings found for menu '%s'...", name); \
                        return {}; \
                    } \
                    const auto menu_json = g_UiSettings[name]; \
                    if(!menu_json.count(this->ui_name)) { \
                        emscripten_log(EM_LOG_CONSOLE, "No UI settings found in menu '%s' for element '%s'...", name, this->ui_name.c_str()); \
                        return {}; \
                    } \
                    el_json = menu_json[this->ui_name]; \
                    break; \
                }

            _MENU_OPTION(Main)
            _MENU_OPTION(Startup)
            _MENU_OPTION(Themes)
            _MENU_OPTION(Settings)
        }

        return el_json;
    }

    inline void LoadPositionAndAlignmentSettings(const nlohmann::json &el_json) {
        if(el_json.count("x")) {
            this->x = el_json["x"].get<u32>();
        }
        if(el_json.count("y")) {
            this->y = el_json["y"].get<u32>();
        }
        if(el_json.count("h_align")) {
            const auto raw_h_align = el_json["h_align"].get<std::string>();

            if(raw_h_align == "left") {
                this->h_align = static_cast<int>(ul::design::HorizontalAlign::Left);
            }
            else if(raw_h_align == "center") {
                this->h_align = static_cast<int>(ul::design::HorizontalAlign::Center);
            }
            else if(raw_h_align == "right") {
                this->h_align = static_cast<int>(ul::design::HorizontalAlign::Right);
            }
            else {
                emscripten_log(EM_LOG_CONSOLE, "Unknown horizontal align value '%s' for element '%s'...", raw_h_align.c_str(), this->ui_name.c_str());
            }
        }
        if(el_json.count("v_align")) {
            const auto raw_v_align = el_json["v_align"].get<std::string>();

            if(raw_v_align == "top") {
                this->v_align = static_cast<int>(ul::design::VerticalAlign::Top);
            }
            else if(raw_v_align == "center") {
                this->v_align = static_cast<int>(ul::design::VerticalAlign::Center);
            }
            else if(raw_v_align == "bottom") {
                this->v_align = static_cast<int>(ul::design::VerticalAlign::Bottom);
            }
            else {
                emscripten_log(EM_LOG_CONSOLE, "Unknown vertical align value '%s' for element '%s'...", raw_v_align.c_str(), this->ui_name.c_str());
            }
        }
    }

    inline void SavePositionAndAlignmentSettings(nlohmann::json &el_json) {
        el_json["x"] = this->x;
        el_json["y"] = this->y;

        switch(static_cast<ul::design::HorizontalAlign>(this->h_align)) {
            case ul::design::HorizontalAlign::Left:
                el_json["h_align"] = "left";
                break;
            case ul::design::HorizontalAlign::Center:
                el_json["h_align"] = "center";
                break;
            case ul::design::HorizontalAlign::Right:
                el_json["h_align"] = "right";
                break;
        }

        switch(static_cast<ul::design::VerticalAlign>(this->v_align)) {
            case ul::design::VerticalAlign::Top:
                el_json["v_align"] = "top";
                break;
            case ul::design::VerticalAlign::Center:
                el_json["v_align"] = "center";
                break;
            case ul::design::VerticalAlign::Bottom:
                el_json["v_align"] = "bottom";
                break;
        }
    }

    inline void LoadAsAssetImage(const std::string &path, const u32 custom_width, const u32 custom_height) {
        if(path.empty()) {
            return;
        }

        void *img_data;
        size_t img_data_size;
        if(path[0] == '?') {
            const auto asset_path = "assets/" + path.substr(1);

            auto f = fopen(asset_path.c_str(), "rb");
            if(f) {
                fseek(f, 0, SEEK_END);
                img_data_size = ftell(f);
                rewind(f);

                img_data = malloc(img_data_size);
                fread(img_data, img_data_size, 1, f);
                fclose(f);
            }
            else {
                emscripten_log(EM_LOG_CONSOLE, "Unable to load asset file '%s'...", asset_path.c_str());
            }
        }
        else {
            int zip_rc;
            for(u32 i = 0; i < std::size(AllowedImageFormats); i++) {
                zip_rc = zip_entry_open(g_ThemeZip, (path + "." + AllowedImageFormats[i]).c_str());
                if(zip_rc == 0) {
                    break;
                }
            }
            if(zip_rc != 0) {
                zip_entry_close(g_ThemeZip);
                return;
            }

            if(zip_entry_read(g_ThemeZip, &img_data, &img_data_size) <= 0) {
                emscripten_log(EM_LOG_CONSOLE, "Unable to read image '%s' from theme ZIP", path.c_str());
                zip_entry_close(g_ThemeZip);
                return;
            }

            zip_entry_close(g_ThemeZip);
        }

        this->LoadAsRawImage(img_data, img_data_size, custom_width, custom_height);
    }

    inline void LoadAsRawImage(void *img_data, const size_t img_data_size, const u32 custom_width, const u32 custom_height) {
        this->el_img.Dispose();

        this->el_img.LoadFrom(img_data, img_data_size, custom_width, custom_height);
    }

    inline void LoadAsText(const std::string &text) {
        this->el_text.text = text;
        this->el_text.font_size = static_cast<int>(ul::design::FontSize::Medium);

        const auto text_json = this->GetSettings();
        if(text_json.count("font_size")) {
            const auto raw_font_size = text_json["font_size"].get<std::string>();

            if(raw_font_size == "small") {
                this->el_text.font_size = static_cast<int>(ul::design::FontSize::Small);
            }
            else if(raw_font_size == "medium") {
                this->el_text.font_size = static_cast<int>(ul::design::FontSize::Medium);
            }
            else if(raw_font_size == "medium-large") {
                this->el_text.font_size = static_cast<int>(ul::design::FontSize::MediumLarge);
            }
            else if(raw_font_size == "large") {
                this->el_text.font_size = static_cast<int>(ul::design::FontSize::Large);
            }
            else {
                emscripten_log(EM_LOG_CONSOLE, "Unknown font size value '%s' for text element '%s'...", raw_font_size.c_str(), this->ui_name.c_str());
            }
        }
    }

    inline void LoadAsDefault() {
        // ...
    }

    inline void Load() {
        while(this->id == 0) {
            this->id = rand();
        }

        if(!this->ui_name.empty()) {
            const auto el_json = this->GetSettings();
            this->LoadPositionAndAlignmentSettings(el_json);
        }

        switch(this->type) {
            case ul::design::ElementType::None: {
                this->LoadAsDefault();
                break;
            }
            case ul::design::ElementType::Image: {
                if(!this->load_ctx.img_path.empty()) {
                    this->LoadAsAssetImage(this->load_ctx.img_path, this->load_ctx.img_custom_width, this->load_ctx.img_custom_height);
                }
                else {
                    this->LoadAsRawImage(this->load_ctx.img_raw_data, this->load_ctx.img_raw_data_size, this->load_ctx.img_custom_width, this->load_ctx.img_custom_height);
                }
                break;
            }
            case ul::design::ElementType::Text: {
                this->LoadAsText(this->load_ctx.text_text);
                break;
            }
        }
    }

    void Reload(const ElementLoadContext ctx) {
        const auto old_ctx = this->load_ctx;
        this->load_ctx = ctx;
        this->load_ctx.img_custom_width = old_ctx.img_custom_width;
        this->load_ctx.img_custom_height = old_ctx.img_custom_height;
        this->Load();
        this->load_ctx = old_ctx;
    }

    inline u32 GetDrawX(const u32 width) {
        auto draw_x = this->x;
        if(static_cast<ul::design::HorizontalAlign>(this->h_align) == ul::design::HorizontalAlign::Center) {
            draw_x = (ul::design::ScreenWidth - width) / 2;
        }
        else if(static_cast<ul::design::HorizontalAlign>(this->h_align) == ul::design::HorizontalAlign::Right) {
            draw_x = ul::design::ScreenWidth - width - x;
        }
        return draw_x;
    }

    inline u32 GetDrawY(const u32 height) {
        auto draw_y = this->y;
        if(static_cast<ul::design::VerticalAlign>(this->v_align) == ul::design::VerticalAlign::Center) {
            draw_y = (ul::design::ScreenHeight - height) / 2;
        }
        else if(static_cast<ul::design::VerticalAlign>(this->v_align) == ul::design::VerticalAlign::Bottom) {
            draw_y = ul::design::ScreenHeight - height - y;
        }
        return draw_y;
    }

    inline void DrawOnWindow() {
        auto draw_list = ImGui::GetWindowDrawList();
        const auto canvas_pos = ImGui::GetCursorScreenPos();

        switch(this->type) {
            case ul::design::ElementType::Image: {
                if(this->el_img.IsValid()) {
                    const auto draw_x = this->GetDrawX(this->el_img.width);
                    const auto draw_y = this->GetDrawY(this->el_img.height);

                    const auto pos_a = ImVec2(canvas_pos.x + draw_x * g_Scale, canvas_pos.y + draw_y * g_Scale);
                    const auto pos_b = ImVec2(pos_a.x + this->el_img.width * g_Scale, pos_a.y + this->el_img.height * g_Scale);
                    
                    draw_list->AddImage((void*)this->el_img.gl_tex, pos_a, pos_b);
                }
                break;
            }
            case ul::design::ElementType::Text: {
                ImFont *draw_font;
                float draw_font_size;
                u32 text_width;
                u32 text_height;
                ProcessText(this->el_text.text, static_cast<ul::design::FontSize>(this->el_text.font_size), text_width, text_height, draw_font, draw_font_size);
                
                const auto draw_x = this->GetDrawX(text_width);
                const auto draw_y = this->GetDrawY(text_height);

                const auto pos = ImVec2(canvas_pos.x + draw_x * g_Scale, canvas_pos.y + draw_y * g_Scale);

                draw_list->AddText(draw_font, draw_font_size * g_Scale, pos, IMGUI_COLOR_TO_U32(g_text_color), this->el_text.text.c_str());
                break;
            }
            default:
                break;
        }
    }

    inline void DrawOnWindowAt(const u32 d_x, const u32 d_y, const u32 width = 0, const u32 height = 0, const ul::design::HorizontalAlign d_h_align = ul::design::HorizontalAlign::Left, const ul::design::VerticalAlign d_v_align = ul::design::VerticalAlign::Top) {
        const auto old_x = this->x;
        this->x = d_x;
        const auto old_y = this->y;
        this->y = d_y;
        const auto old_h_align = this->h_align;
        this->h_align = static_cast<int>(d_h_align);
        const auto old_v_align = this->v_align;
        this->v_align = static_cast<int>(d_v_align);
        const auto old_width = this->el_img.width;
        if(width > 0) {
            this->el_img.width = width;
        }
        const auto old_height = this->el_img.height;
        if(height > 0) {
            this->el_img.height = height;
        }
        this->DrawOnWindow();
        this->el_img.width = old_width;
        this->el_img.height = old_height;
        this->x = old_x;
        this->y = old_y;
        this->h_align = old_h_align;
        this->v_align = old_v_align;
    }

    inline bool ImageIsBuiltin() {
        return this->load_ctx.img_path.at(0) == '?';
    }

    inline bool SaveAsImage(zip_t *theme_zip) {
        if(!this->ImageIsBuiltin() && (this->el_img.width > 0)) {
            GLuint fb;
            glGenFramebuffers(1, &fb);
            glBindFramebuffer(GL_FRAMEBUFFER, fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->el_img.gl_tex, 0);

            auto pixels = reinterpret_cast<u8*>(malloc(this->el_img.width * this->el_img.height * 4));
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(0, 0, this->el_img.width, this->el_img.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

            glDeleteFramebuffers(1, &fb);

            int png_ptr_size;
            auto png_ptr = stbi_write_png_to_mem(pixels, this->el_img.width * 4, this->el_img.width, this->el_img.height, 4, &png_ptr_size);
            return SaveThemeAsset(theme_zip, this->load_ctx.img_path + ".png", png_ptr, png_ptr_size);
        }

        return true;
    }

    inline bool SaveAsText(zip_t *theme_zip) {
        auto text_json = g_UiSettings[ul::design::MenuSettingsNames[static_cast<u32>(this->menu)]][this->ui_name];

        switch(static_cast<ul::design::FontSize>(this->el_text.font_size)) {
            case ul::design::FontSize::Small:
                text_json["font_size"] = "small";
                break;
            case ul::design::FontSize::Medium:
                text_json["font_size"] = "medium";
                break;
            case ul::design::FontSize::MediumLarge:
                text_json["font_size"] = "medium-large";
                break;
            case ul::design::FontSize::Large:
                text_json["font_size"] = "large";
                break;
        }

        return true;
    }

    inline bool SaveAsDefault(zip_t *theme_zip) {
        // ...
        return true;
    }

    inline bool Save(zip_t *theme_zip) {
        if(!this->ui_name.empty() && (this->menu != ul::design::MenuType::None)) {
            this->SavePositionAndAlignmentSettings(g_UiSettings[ul::design::MenuSettingsNames[static_cast<u32>(this->menu)]][this->ui_name]);
        }

        switch(this->type) {
            case ul::design::ElementType::None:
                return this->SaveAsDefault(theme_zip);
            case ul::design::ElementType::Image:
                return this->SaveAsImage(theme_zip);
            case ul::design::ElementType::Text:
                return this->SaveAsText(theme_zip);
        }
    }
};

Element elem_theme_icon(ul::design::MenuType::None, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("theme/Icon"));

Element elem_background(ul::design::MenuType::None, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Background"));

Element elem_main_top_menu_default_bg(ul::design::MenuType::Main, ul::design::ElementType::Image, "top_menu_bg", ElementLoadContext::ForAssetImage("ui/Main/TopMenuBackground/Default"));
Element elem_main_top_menu_app_bg(ul::design::MenuType::Main, ul::design::ElementType::Image, "top_menu_bg", ElementLoadContext::ForAssetImage("ui/Main/TopMenuBackground/Application"));
Element elem_main_top_menu_hb_bg(ul::design::MenuType::Main, ul::design::ElementType::Image, "top_menu_bg", ElementLoadContext::ForAssetImage("ui/Main/TopMenuBackground/Homebrew"));
Element elem_main_top_menu_folder_bg(ul::design::MenuType::Main, ul::design::ElementType::Image, "top_menu_bg", ElementLoadContext::ForAssetImage("ui/Main/TopMenuBackground/Folder"));

Element elem_main_logo_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "logo_top_icon", ElementLoadContext::ForAssetImage("?Logo.png", 90, 90));

Element elem_main_connection_none_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "connection_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Connection/None"));
Element elem_main_connection_0_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "connection_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Connection/0"));
Element elem_main_connection_1_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "connection_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Connection/1"));
Element elem_main_connection_2_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "connection_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Connection/2"));
Element elem_main_connection_3_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "connection_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Connection/3"));

Element elem_main_time_text(ul::design::MenuType::Main, ul::design::ElementType::Text, "time_text", ElementLoadContext::ForText("04:20"));
Element elem_main_date_text(ul::design::MenuType::Main, ul::design::ElementType::Text, "date_text", ElementLoadContext::ForText("15/06 (Sat)"));
Element elem_main_battery_text(ul::design::MenuType::Main, ul::design::ElementType::Text, "battery_text", ElementLoadContext::ForText("69%"));

Element elem_main_battery_10_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/10"));
Element elem_main_battery_20_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/20"));
Element elem_main_battery_30_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/30"));
Element elem_main_battery_40_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/40"));
Element elem_main_battery_50_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/50"));
Element elem_main_battery_60_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/60"));
Element elem_main_battery_70_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/70"));
Element elem_main_battery_80_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/80"));
Element elem_main_battery_90_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/90"));
Element elem_main_battery_100_top_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/100"));

Element elem_main_battery_top_icon_charging(ul::design::MenuType::Main, ul::design::ElementType::Image, "battery_top_icon", ElementLoadContext::ForAssetImage("ui/Main/TopIcon/Battery/Charging"));
Element elem_main_input_bar_bg(ul::design::MenuType::Main, ul::design::ElementType::Image, "input_bar", ElementLoadContext::ForAssetImage("ui/Main/InputBarBackground"));
Element elem_main_cur_path_text(ul::design::MenuType::Main, ul::design::ElementType::Text, "cur_path_text", ElementLoadContext::ForText("/Sample folder"));
Element elem_main_cur_entry_main_text(ul::design::MenuType::Main, ul::design::ElementType::Text, "cur_entry_main_text", ElementLoadContext::ForText("Entry main text"));
Element elem_main_cur_entry_sub_text(ul::design::MenuType::Main, ul::design::ElementType::Text, "cur_entry_sub_text", ElementLoadContext::ForText("Entry sub text"));
Element elem_main_entry_menu_bg(ul::design::MenuType::Main, ul::design::ElementType::Image, "entry_menu_bg", ElementLoadContext::ForAssetImage("ui/Main/EntryMenuBackground"));
Element elem_main_entry_menu_left_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "entry_menu_left_icon", ElementLoadContext::ForAssetImage("ui/Main/EntryMenuLeftIcon"));
Element elem_main_entry_menu_right_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "entry_menu_right_icon", ElementLoadContext::ForAssetImage("ui/Main/EntryMenuRightIcon"));

Element elem_main_entry_menu(ul::design::MenuType::Main, ul::design::ElementType::None, "entry_menu", {});

Element elem_main_album_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/Album"));
Element elem_main_controllers_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/Controllers"));
Element elem_main_default_app_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/DefaultApplication"));
Element elem_main_default_hb_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/DefaultHomebrew"));
Element elem_main_empty_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/Empty"));
Element elem_main_folder_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/Folder"));
Element elem_main_mii_edit_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/MiiEdit"));
Element elem_main_settings_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/Settings"));
Element elem_main_themes_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/Themes"));
Element elem_main_web_browser_entry_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/EntryIcon/WebBrowser"));

Element elem_main_album_quick_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/QuickIcon/Album"));
Element elem_main_controllers_quick_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/QuickIcon/Controllers"));
Element elem_main_mii_edit_quick_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/QuickIcon/MiiEdit"));
Element elem_main_power_quick_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/QuickIcon/Power"));
Element elem_main_settings_quick_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/QuickIcon/Settings"));
Element elem_main_themes_quick_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/QuickIcon/Themes"));
Element elem_main_web_browser_quick_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/QuickIcon/WebBrowser"));

Element elem_main_border_over_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/OverIcon/Border"));
Element elem_main_cursor_over_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/OverIcon/Cursor"));
Element elem_main_hb_takeover_app_over_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/OverIcon/HomebrewTakeoverApplication"));
Element elem_main_selected_over_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/OverIcon/Selected"));
Element elem_main_suspended_over_icon(ul::design::MenuType::Main, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Main/OverIcon/Suspended"));

Element elem_startup_info_text(ul::design::MenuType::Startup, ul::design::ElementType::Text, "info_text", ElementLoadContext::ForText("Welcome! Please select a user."));
Element elem_startup_users_menu(ul::design::MenuType::Startup, ul::design::ElementType::None, "users_menu", {});

Element elem_themes_info_text(ul::design::MenuType::Themes, ul::design::ElementType::Text, "info_text", ElementLoadContext::ForText("Available themes"));
Element elem_themes_themes_menu(ul::design::MenuType::Themes, ul::design::ElementType::None, "themes_menu", {});

Element elem_settings_info_text(ul::design::MenuType::Settings, ul::design::ElementType::Text, "info_text", ElementLoadContext::ForText("Console & uLaunch settings"));
Element elem_settings_settings_menu(ul::design::MenuType::Settings, ul::design::ElementType::None, "settings_menu", {});
Element elem_settings_setting_editable_icon(ul::design::MenuType::Settings, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Settings/SettingEditableIcon"));
Element elem_settings_setting_non_editable_icon(ul::design::MenuType::Settings, ul::design::ElementType::Image, "", ElementLoadContext::ForAssetImage("ui/Settings/SettingNonEditableIcon"));

Element *g_AllElements[] = {
    &elem_theme_icon,

    &elem_background,
    &elem_main_logo_top_icon,

    &elem_main_connection_none_top_icon,
    &elem_main_connection_0_top_icon,
    &elem_main_connection_1_top_icon,
    &elem_main_connection_2_top_icon,
    &elem_main_connection_3_top_icon,

    &elem_main_time_text,
    &elem_main_date_text,
    &elem_main_battery_text,
    
    &elem_main_battery_10_top_icon,
    &elem_main_battery_20_top_icon,
    &elem_main_battery_30_top_icon,
    &elem_main_battery_40_top_icon,
    &elem_main_battery_50_top_icon,
    &elem_main_battery_60_top_icon,
    &elem_main_battery_70_top_icon,
    &elem_main_battery_80_top_icon,
    &elem_main_battery_90_top_icon,
    &elem_main_battery_100_top_icon,

    &elem_main_battery_top_icon_charging,
    &elem_main_input_bar_bg,
    &elem_main_cur_path_text,
    &elem_main_cur_entry_main_text,
    &elem_main_cur_entry_sub_text,
    &elem_main_entry_menu_bg,
    &elem_main_entry_menu_left_icon,
    &elem_main_entry_menu_right_icon,

    &elem_main_entry_menu,

    &elem_main_top_menu_default_bg,
    &elem_main_top_menu_folder_bg,
    &elem_main_top_menu_app_bg,
    &elem_main_top_menu_hb_bg,

    &elem_main_album_entry_icon,
    &elem_main_album_quick_icon,
    &elem_main_border_over_icon,
    &elem_main_controllers_entry_icon,
    &elem_main_controllers_quick_icon,
    &elem_main_cursor_over_icon,
    &elem_main_default_app_entry_icon,
    &elem_main_default_hb_entry_icon,
    &elem_main_empty_entry_icon,
    &elem_main_folder_entry_icon,
    &elem_main_hb_takeover_app_over_icon,
    &elem_main_mii_edit_entry_icon,
    &elem_main_mii_edit_quick_icon,
    &elem_main_power_quick_icon,
    &elem_main_selected_over_icon,
    &elem_main_settings_entry_icon,
    &elem_main_settings_quick_icon,
    &elem_main_suspended_over_icon,
    &elem_main_themes_entry_icon,
    &elem_main_themes_quick_icon,
    &elem_main_web_browser_entry_icon,
    &elem_main_web_browser_quick_icon,

    &elem_startup_info_text,
    &elem_startup_users_menu,

    &elem_themes_info_text,
    &elem_themes_themes_menu,

    &elem_settings_info_text,
    &elem_settings_settings_menu,
    &elem_settings_setting_editable_icon,
    &elem_settings_setting_non_editable_icon
};

struct SoundFile {
    std::string path;
    void *data;
    size_t data_size;

    SoundFile(const std::string &path) : path(path), data(nullptr), data_size(0) {}
};

SoundFile bgm_main("sound/Main/Bgm.mp3");
SoundFile sfx_main_post_suspend("sound/Main/PostSuspend.wav");
SoundFile sfx_main_cursor_move("sound/Main/CursorMove.wav");
SoundFile sfx_main_page_move("sound/Main/PageMove.wav");
SoundFile sfx_main_entry_select("sound/Main/EntrySelect.wav");
SoundFile sfx_main_entry_move("sound/Main/EntryMove.wav");
SoundFile sfx_main_entry_swap("sound/Main/EntrySwap.wav");
SoundFile sfx_main_entry_cancel_select("sound/Main/EntryCancelSelect.wav");
SoundFile sfx_main_entry_move_into("sound/Main/EntryMoveInto.wav");
SoundFile sfx_main_home_press("sound/Main/HomePress.wav");
SoundFile sfx_main_logoff("sound/Main/Logoff.wav");
SoundFile sfx_main_launch_app("sound/Main/LaunchApplication.wav");
SoundFile sfx_main_launch_hb("sound/Main/LaunchHomebrew.wav");
SoundFile sfx_main_close_suspended("sound/Main/CloseSuspended.wav");
SoundFile sfx_main_open_folder("sound/Main/OpenFolder.wav");
SoundFile sfx_main_close_folder("sound/Main/CloseFolder.wav");
SoundFile sfx_main_open_mii_edit("sound/Main/OpenMiiEdit.wav");
SoundFile sfx_main_open_web_browser("sound/Main/OpenWebBrowser.wav");
SoundFile sfx_main_open_user_page("sound/Main/OpenUserPage.wav");
SoundFile sfx_main_open_settings("sound/Main/OpenSettings.wav");
SoundFile sfx_main_open_themes("sound/Main/OpenThemes.wav");
SoundFile sfx_main_open_controllers("sound/Main/OpenControllers.wav");
SoundFile sfx_main_open_album("sound/Main/OpenAlbum.wav");
SoundFile sfx_main_open_quick_menu("sound/Main/OpenQuickMenu.wav");
SoundFile sfx_main_close_quick_menu("sound/Main/CloseQuickMenu.wav");
SoundFile sfx_main_resume_app("sound/Main/ResumeApplication.wav");
SoundFile sfx_main_create_folder("sound/Main/CreateFolder.wav");
SoundFile sfx_main_create_hb_entry("sound/Main/CreateHomebrewEntry.wav");
SoundFile sfx_main_entry_remove("sound/Main/EntryRemove.wav");
SoundFile sfx_main_error("sound/Main/Error.wav");

SoundFile bgm_startup("sound/Startup/Bgm.mp3");
SoundFile sfx_startup_user_create("sound/Startup/UserCreate.wav");
SoundFile sfx_startup_user_select("sound/Startup/UserSelect.wav");

SoundFile bgm_themes("sound/Themes/Bgm.mp3");
SoundFile sfx_themes_theme_change("sound/Themes/ThemeChange.wav");
SoundFile sfx_themes_back("sound/Themes/Back.wav");

SoundFile bgm_settings("sound/Settings/Bgm.mp3");
SoundFile sfx_settings_setting_edit("sound/Settings/SettingEdit.wav");
SoundFile sfx_settings_setting_save("sound/Settings/SettingSave.wav");
SoundFile sfx_settings_back("sound/Settings/Back.wav");

SoundFile *g_AllSoundFiles[] = {
    &bgm_main,
    &sfx_main_post_suspend,
    &sfx_main_cursor_move,
    &sfx_main_page_move,
    &sfx_main_entry_select,
    &sfx_main_entry_move,
    &sfx_main_entry_swap,
    &sfx_main_entry_cancel_select,
    &sfx_main_entry_move_into,
    &sfx_main_home_press,
    &sfx_main_logoff,
    &sfx_main_launch_app,
    &sfx_main_launch_hb,
    &sfx_main_close_suspended,
    &sfx_main_open_folder,
    &sfx_main_close_folder,
    &sfx_main_open_mii_edit,
    &sfx_main_open_web_browser,
    &sfx_main_open_user_page,
    &sfx_main_open_settings,
    &sfx_main_open_themes,
    &sfx_main_open_controllers,
    &sfx_main_open_album,
    &sfx_main_open_quick_menu,
    &sfx_main_close_quick_menu,
    &sfx_main_resume_app,
    &sfx_main_create_folder,
    &sfx_main_create_hb_entry,
    &sfx_main_entry_remove,
    &sfx_main_error,

    &bgm_startup,
    &sfx_startup_user_create,
    &sfx_startup_user_select,

    &bgm_themes,
    &sfx_themes_theme_change,
    &sfx_themes_back,

    &bgm_settings,
    &sfx_settings_setting_edit,
    &sfx_settings_setting_save,
    &sfx_settings_back
};

extern "C" EMSCRIPTEN_KEEPALIVE void cpp_LoadElementImage(Element *elem, void *img_data, const size_t img_data_size) {
    elem->Reload(ElementLoadContext::ForRawImage(img_data, img_data_size));
}

EM_JS(void, LoadElementImage, (Element *elem), {
    var input = document.createElement("input");
    input.type = "file";
    input.id = "file-selector";
    input.accept = ".jpg,.jpeg,.png";
    input.addEventListener("change", (event) => {
        var file = event.target.files[0];
        var reader = new FileReader();
        reader.addEventListener("load", () => {
            var file_data = new Uint8Array(reader.result);
            var file_ptr = Module._malloc(file_data.length);
            Module.HEAPU8.set(file_data, file_ptr);
            Module.ccall("cpp_LoadElementImage", null, ["number", "number", "number"], [elem, file_ptr, file_data.length]);
        }, false);
        reader.readAsArrayBuffer(file);
    });

    if(document.createEvent) {
        var event = document.createEvent("MouseEvents");
        event.initEvent("click", true, true);
        input.dispatchEvent(event);
    }
    else {
        input.click();
    }
});

constexpr ImWchar StandardGlyphRanges[] = {
    0x0020, 0x00FF, // Basic Latin + Latin Supplement
    0x0000
};

constexpr ImWchar ExtendedGlyphRanges[] = {
    0x25A0, 0xE200, // Special Nintendo buttons
    0x0000
};

void ReloadFonts() {
    auto &io = ImGui::GetIO();

    #define _LOAD_ASSET_FONT_FOR_SIZE(name, size) { \
        ImFontConfig config; \
        config.MergeMode = true; \
        g_FontStandard_##name = io.Fonts->AddFontFromMemoryTTF(g_FontData, g_FontDataSize, size * g_Scale, nullptr, StandardGlyphRanges); \
        g_FontExtended_##name = io.Fonts->AddFontFromFileTTF("assets/FontExtended.ttf", size * g_Scale, &config, ExtendedGlyphRanges); \
    }

    if(g_FontData != nullptr) {
        _LOAD_ASSET_FONT_FOR_SIZE(Small, 27.0f)
        _LOAD_ASSET_FONT_FOR_SIZE(Medium, 30.0f)
        _LOAD_ASSET_FONT_FOR_SIZE(MediumLarge, 37.0f)
        _LOAD_ASSET_FONT_FOR_SIZE(Large, 45.0f)

        io.Fonts->Build();
    }
}

extern "C" EMSCRIPTEN_KEEPALIVE void cpp_LoadFont(void *font_data, const size_t font_data_size) {
    if(g_FontData != nullptr) {
        free(g_FontData);
    }

    g_FontData = font_data;
    g_FontDataSize = font_data_size;
    ReloadFonts();
}

EM_JS(void, LoadFont, (), {
    var input = document.createElement("input");
    input.type = "file";
    input.id = "file-selector";
    input.accept = ".ttf";
    input.addEventListener("change", (event) => {
        var file = event.target.files[0];
        var reader = new FileReader();
        reader.addEventListener("load", () => {
            var file_data = new Uint8Array(reader.result);
            var file_ptr = Module._malloc(file_data.length);
            Module.HEAPU8.set(file_data, file_ptr);
            Module.ccall("cpp_LoadFont", null, ["number", "number"], [file_ptr, file_data.length]);
        }, false);
        reader.readAsArrayBuffer(file);
    });

    if(document.createEvent) {
        var event = document.createEvent("MouseEvents");
        event.initEvent("click", true, true);
        input.dispatchEvent(event);
    }
    else {
        input.click();
    }
});

bool ReloadFromTheme() {
    if(!LoadThemeJsonAsset(ul::design::ManifestPath, g_Manifest)) {
        return false;
    }
    if(g_Manifest.count("format_version")) {
        const auto fmt_version = g_Manifest["format_version"].get<u32>();
        if(fmt_version < ul::design::CurrentFormatVersion) {
            emscripten_log(EM_LOG_CONSOLE, "Loaded theme is outdated (theme version %d vs current version %d)", fmt_version, ul::design::CurrentFormatVersion);
        }
    }
    else {
        emscripten_log(EM_LOG_CONSOLE, "Unable to find manifest format version...");
    }

    #define _MANIFEST_LOAD_FIELD(var, name) { \
        if(g_Manifest.count(name)) { \
            const auto tmp = g_Manifest[name].get<std::string>(); \
            strncpy(var, tmp.c_str(), tmp.length()); \
        } \
        else { \
            emscripten_log(EM_LOG_CONSOLE, "Unable to find manifest field '" name "'..."); \
        } \
    }
    _MANIFEST_LOAD_FIELD(g_ManifestName, "name")
    _MANIFEST_LOAD_FIELD(g_ManifestDescription, "description")
    _MANIFEST_LOAD_FIELD(g_ManifestRelease, "release")
    _MANIFEST_LOAD_FIELD(g_ManifestAuthor, "author")

    if(!LoadThemeJsonAsset(ul::design::UiSettingsPath, g_UiSettings)) {
        return false;
    }

    #define _UI_LOAD_COLOR(name) { \
        if(g_UiSettings.count(#name)) { \
            g_##name = ParseHexColor(g_UiSettings[#name].get<std::string>()); \
        } \
        else { \
            emscripten_log(EM_LOG_CONSOLE, "Unable to find UI color '" #name "'..."); \
        } \
    }
    _UI_LOAD_COLOR(text_color)
    _UI_LOAD_COLOR(menu_focus_color)
    _UI_LOAD_COLOR(menu_bg_color)
    _UI_LOAD_COLOR(dialog_title_color)
    _UI_LOAD_COLOR(dialog_cnt_color)
    _UI_LOAD_COLOR(dialog_opt_color)
    _UI_LOAD_COLOR(dialog_color)
    _UI_LOAD_COLOR(dialog_over_color)

    if(g_UiSettings.count("suspended_app_final_alpha")) {
        g_suspended_app_final_alpha = g_UiSettings["suspended_app_final_alpha"].get<int>();
    }
    else {
        emscripten_log(EM_LOG_CONSOLE, "Unable to find suspended app final alpha...");
    }

    for(u32 i = 0; i < std::size(g_AllElements); i++) {
        auto elem = g_AllElements[i];
        elem->Load();
    }

    auto zip_rc = zip_entry_open(g_ThemeZip, "ui/Font.ttf");
    if(zip_rc == 0) {
        void *ttf_data;
        size_t ttf_data_size;
        if(zip_entry_read(g_ThemeZip, &ttf_data, &ttf_data_size) <= 0) {
            zip_entry_close(g_ThemeZip);
            emscripten_log(EM_LOG_CONSOLE, "Unable to read theme TTF font...");
            zip_close(g_ThemeZip);
            return false;
        }

        if(g_FontData != nullptr) {
            free(g_FontData);
        }

        g_FontData = ttf_data;
        g_FontDataSize = ttf_data_size;
        zip_entry_close(g_ThemeZip);
    }

    if(LoadThemeJsonAsset(ul::design::SoundSettingsPath, g_SoundSettings)) {
        g_SoundSettingsLoaded = true;

        #define _LOAD_BGM_FIELDS(json, v_bgm_loop, v_bgm_fade_in_ms, v_bgm_fade_out_ms) { \
            if(json.count("bgm_loop")) { \
                v_bgm_loop = json["bgm_loop"].get<bool>(); \
            } \
            if(json.count("bgm_fade_in_ms")) { \
                v_bgm_fade_in_ms = json["bgm_fade_in_ms"].get<int>(); \
            } \
            if(json.count("bgm_fade_out_ms")) { \
                v_bgm_fade_out_ms = json["bgm_fade_out_ms"].get<int>(); \
            } \
        }

        _LOAD_BGM_FIELDS(g_SoundSettings, g_bgm_loop, g_bgm_fade_in_ms, g_bgm_fade_out_ms);

        #define _LOAD_MENU_BGM_FIELDS(menu, bgm_loop, bgm_fade_in_ms, bgm_fade_out_ms) { \
            if(g_SoundSettings.count(menu)) { \
                auto menu_json = g_SoundSettings[menu]; \
                _LOAD_BGM_FIELDS(menu_json, bgm_loop, bgm_fade_in_ms, bgm_fade_out_ms); \
            } \
        }

        _LOAD_MENU_BGM_FIELDS("main_menu", g_main_bgm_loop, g_main_bgm_fade_in_ms, g_main_bgm_fade_out_ms);
        _LOAD_MENU_BGM_FIELDS("startup_menu", g_startup_bgm_loop, g_startup_bgm_fade_in_ms, g_startup_bgm_fade_out_ms);
        _LOAD_MENU_BGM_FIELDS("themes_menu", g_themes_bgm_loop, g_themes_bgm_fade_in_ms, g_themes_bgm_fade_out_ms);
        _LOAD_MENU_BGM_FIELDS("settings_menu", g_settings_bgm_loop, g_settings_bgm_fade_in_ms, g_settings_bgm_fade_out_ms);
    }

    for(u32 i = 0; i < std::size(g_AllSoundFiles); i++) {
        auto &sound_file = g_AllSoundFiles[i];
        if(sound_file->data != nullptr) {
            free(sound_file->data);
        }

        auto zip_rc = zip_entry_open(g_ThemeZip, sound_file->path.c_str());
        if(zip_rc == 0) {
            void *sound_data;
            size_t sound_data_size;
            if(zip_entry_read(g_ThemeZip, &sound_file->data, &sound_file->data_size) <= 0) {
                zip_entry_close(g_ThemeZip);
                emscripten_log(EM_LOG_CONSOLE, "Unable to read sound file at '%s'...", sound_file->path.c_str());
                zip_close(g_ThemeZip);
                return false;
            }

            zip_entry_close(g_ThemeZip);
        }
    }

    return true;
}

extern "C" EMSCRIPTEN_KEEPALIVE void cpp_LoadThemeZip(void *theme_zip_data, const size_t theme_zip_data_size) {
    if(g_ThemeZip != nullptr) {
        zip_close(g_ThemeZip);
    }

    if(g_ThemeZipData != nullptr) {
        free(g_ThemeZipData);
    }

    g_ThemeZipData = theme_zip_data;
    g_ThemeZipDataSize = theme_zip_data_size;
    g_ThemeZip = zip_stream_open(reinterpret_cast<const char*>(theme_zip_data), theme_zip_data_size, 0, 'r');

    ReloadFromTheme();
}

void LoadDefaultThemeZip() {
    auto f = fopen("assets/default-theme.zip", "rb");
    if(f) {
        fseek(f, 0, SEEK_END);
        const auto theme_data_size = ftell(f);
        rewind(f);

        auto theme_data = malloc(theme_data_size);
        fread(theme_data, theme_data_size, 1, f);
        fclose(f);

        cpp_LoadThemeZip(theme_data, theme_data_size);
    }
    else {
        emscripten_log(EM_LOG_CONSOLE, "Unable to open default theme ZIP...");
    }
}

EM_JS(void, LoadThemeZip, (), {
    var input = document.createElement("input");
    input.type = "file";
    input.id = "file-selector";
    input.accept = ".zip";
    input.addEventListener("change", (event) => {
        var file = event.target.files[0];
        var reader = new FileReader();
        reader.addEventListener("load", () => {
            var file_data = new Uint8Array(reader.result);
            var file_ptr = Module._malloc(file_data.length);
            Module.HEAPU8.set(file_data, file_ptr);
            Module.ccall("cpp_LoadThemeZip", null, ["number", "number"], [file_ptr, file_data.length]);
        }, false);
        reader.readAsArrayBuffer(file);
    });

    if(document.createEvent) {
        var event = document.createEvent("MouseEvents");
        event.initEvent("click", true, true);
        input.dispatchEvent(event);
    }
    else {
        input.click();
    }
});

EM_JS(void, DoSaveThemeZip, (void *theme_zip_data, const size_t theme_zip_data_size), {
    var data_array = Module.HEAPU8.subarray(theme_zip_data, theme_zip_data + theme_zip_data_size);
    var pom = document.createElement("a");
    pom.setAttribute("href", window.URL.createObjectURL(new Blob([data_array], {type: "octet/stream"})));
    pom.setAttribute("download", "theme.zip");

    if(document.createEvent) {
        var event = document.createEvent("MouseEvents");
        event.initEvent("click", true, true);
        pom.dispatchEvent(event);
    }
    else {
        pom.click();
    }
});

void SaveThemeZip() {
    auto theme_zip = zip_stream_open(nullptr, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

    for(auto &elem: g_AllElements)  {
        if(!elem->Save(theme_zip)) {
            emscripten_log(EM_LOG_CONSOLE, "Unable to save element...");
            zip_stream_close(theme_zip);
            return;
        }
    }

    for(auto &sound_file: g_AllSoundFiles)  {
        if(sound_file->data != nullptr) {
            if(!SaveThemeAsset(theme_zip, sound_file->path, sound_file->data, sound_file->data_size)) {
                emscripten_log(EM_LOG_CONSOLE, "Unable to save sound file at '%s'...", sound_file->path.c_str());
                zip_stream_close(theme_zip);
                return;
            }
        }
    }

    g_Manifest["format_version"] = 2;
    g_Manifest["name"] = g_ManifestName;
    g_Manifest["description"] = g_ManifestDescription;
    g_Manifest["author"] = g_ManifestAuthor;
    g_Manifest["release"] = g_ManifestRelease;

    if(!SaveThemeJsonAsset(theme_zip, ul::design::ManifestPath, g_Manifest)) {
        emscripten_log(EM_LOG_CONSOLE, "Unable to save manifest JSON...");
        zip_stream_close(theme_zip);
        return;
    }

    #define _UI_SAVE_COLOR(name) { \
        g_UiSettings[#name] = GenerateHexColor(g_##name); \
    }
    _UI_SAVE_COLOR(text_color)
    _UI_SAVE_COLOR(menu_focus_color)
    _UI_SAVE_COLOR(menu_bg_color)
    _UI_SAVE_COLOR(dialog_title_color)
    _UI_SAVE_COLOR(dialog_cnt_color)
    _UI_SAVE_COLOR(dialog_opt_color)
    _UI_SAVE_COLOR(dialog_color)
    _UI_SAVE_COLOR(dialog_over_color)

    g_UiSettings["suspended_app_final_alpha"] = g_suspended_app_final_alpha;

    if(!SaveThemeJsonAsset(theme_zip, ul::design::UiSettingsPath, g_UiSettings)) {
        emscripten_log(EM_LOG_CONSOLE, "Unable to save UI settings JSON...");
        zip_stream_close(theme_zip);
        return;
    }

    #define _SAVE_BGM_FIELDS(json) { \
        if(g_bgm_loop.has_value()) { \
            json["bgm_loop"] = g_bgm_loop.value(); \
        } \
        if(g_bgm_fade_in_ms.has_value()) { \
            json["bgm_fade_in_ms"] = g_bgm_fade_in_ms.value(); \
        } \
        if(g_bgm_fade_out_ms.has_value()) { \
            json["bgm_fade_out_ms"] = g_bgm_fade_out_ms.value(); \
        } \
    }

    if(g_SoundSettingsLoaded) {
        _SAVE_BGM_FIELDS(g_SoundSettings);
        _SAVE_BGM_FIELDS(g_SoundSettings["main_menu"]);
        _SAVE_BGM_FIELDS(g_SoundSettings["startup_menu"]);
        _SAVE_BGM_FIELDS(g_SoundSettings["themes_menu"]);
        _SAVE_BGM_FIELDS(g_SoundSettings["settings_menu"]);

        if(!SaveThemeJsonAsset(theme_zip, ul::design::SoundSettingsPath, g_SoundSettings)) {
            emscripten_log(EM_LOG_CONSOLE, "Unable to save sound settings JSON...");
            zip_stream_close(theme_zip);
            return;
        }
    }

    if(g_FontData != nullptr) {
        if(!SaveThemeAsset(theme_zip, "ui/Font.ttf", g_FontData, g_FontDataSize)) {
            emscripten_log(EM_LOG_CONSOLE, "Unable to save UI font...");
            zip_stream_close(theme_zip);
            return;
        }
    }

    void *zip_buf;
    size_t zip_buf_size;
    if(zip_stream_copy(theme_zip, &zip_buf, &zip_buf_size) <= 0) {
        emscripten_log(EM_LOG_CONSOLE, "Unable to save theme ZIP...");
        zip_stream_close(theme_zip);
        return;
    }

    zip_stream_close(theme_zip);

    DoSaveThemeZip(zip_buf, zip_buf_size);
}

void DrawItemMenu(const std::vector<Element*> &icons, const std::vector<std::string> &texts, const u32 focus_idx, const u32 base_x, const u32 base_y, const u32 width, const u32 item_height, const ImU32 bg_color) {
    DrawRectangle(0, 0, ul::design::ScreenWidth, ul::design::ScreenHeight, bg_color);

    DrawRectangle(base_x, base_y, width, item_height * icons.size(), IMGUI_COLOR_TO_U32(g_menu_bg_color));
    DrawRectangle(base_x, base_y + focus_idx * item_height, width, item_height, IMGUI_COLOR_TO_U32(g_menu_focus_color));

    u32 cur_y = base_y;
    for(u32 i = 0; i < icons.size(); i++) {
        auto &icon = icons.at(i);

        u32 icon_width = 0;
        if(icon != nullptr) {
            const auto factor = (float)icon->el_img.height / (float)icon->el_img.width;
            icon_width = (u32)(item_height * 0.8f);
            auto icon_height = icon_width;
            if(factor < 1) {
                icon_height = (u32)(icon_width * factor);
            }
            else {
                icon_width = (u32)(icon_height * factor);
            }

            icon->DrawOnWindowAt(base_x + ul::design::ItemMenuDefaultIconMargin, cur_y + (u32)((double)(item_height - icon_height) / 2.0f), icon_width, icon_height);
        }

        const auto &text = texts.at(i);
        u32 text_width;
        u32 text_height;
        ImFont *dummy_font;
        float dummy_size;
        ProcessText(text, ul::design::FontSize::MediumLarge, text_width, text_height, dummy_font, dummy_size);

        DrawTextAt(text, ul::design::FontSize::MediumLarge, base_x + ul::design::ItemMenuDefaultIconMargin + icon_width + ul::design::ItemMenuDefaultTextMargin, cur_y + (u32)((double)(item_height - text_height) / 2.0f));

        cur_y += item_height;
    }
}

void DrawItemMenuElement(Element &elem, const std::vector<Element*> &icons, const std::vector<std::string> &texts, const u32 focus_idx, const u32 width, const u32 item_height) {
    const auto x = elem.GetDrawX(width);
    const auto y = elem.GetDrawY(item_height * icons.size());
    DrawItemMenu(icons, texts, focus_idx, x, y, width, item_height, 0);
}

constexpr const char *HorizontalAlignNames[] = {
    "Left",
    "Center",
    "Right"
};

constexpr const char *VerticalAlignNames[] = {
    "Top",
    "Center",
    "Bottom"
};

constexpr const char *FontSizeNames[] = {
    "Small",
    "Medium",
    "Medium-Large",
    "Large"
};

constexpr const char *TopMenuBackgroundNames[] = {
    "Default",
    "Application",
    "Homebrew",
    "Folder"
};

constexpr const char *ConnectionTopIconNames[] = {
    "None",
    "0",
    "1",
    "2",
    "3"
};

constexpr const char *BatteryTopIconNames[] = {
    "10",
    "20",
    "30",
    "40",
    "50",
    "60",
    "70",
    "80",
    "90",
    "100"
};

int g_VisibleTopMenuBackground = 0;
int g_VisibleConnectionTopIcon = 0;
int g_VisibleBatteryTopIcon = 0;
bool g_DrawBatteryCharging = false;
bool g_DrawQuickMenu = false;

namespace {

    constexpr float WindowWidthFactor = 0.8;
    constexpr float WindowHeightFactor = 0.8;

    void OnCanvasDimensionsChanged() {
        glfwSetWindowSize(g_Window, g_Width, g_Height);
        ImGui::SetCurrentContext(ImGui::GetCurrentContext());
    }

    void LoadColorPicker(const char *label, ImColor *color) {
        if(ImGui::CollapsingHeader(label)) {
            ImGui::ColorPicker4((std::string("##clr") + label).c_str(), (float*)&color->Value, ImGuiColorEditFlags_Uint8);
        }
    }

    void LoadEditableElement(Element *elem, const std::vector<Element*> &modifiable_elems = {}) {
        const auto has_settings = !elem->ui_name.empty();
        std::string name;
        if(has_settings) {
            name = elem->ui_name;
            if(elem->type == ul::design::ElementType::Image) {
                name += " (" + elem->load_ctx.img_path + ", " + std::to_string(elem->el_img.width) + "x" + std::to_string(elem->el_img.height) + ")";
            }
        }
        else {
            name = elem->load_ctx.img_path + ", " + std::to_string(elem->el_img.width) + "x" + std::to_string(elem->el_img.height);
        }
        const auto id_name = std::to_string(elem->id);
        if(ImGui::CollapsingHeader(name.c_str())) {
            ImGui::Indent();
            if(has_settings) {
                if(ImGui::InputInt(("X##inc" + id_name).c_str(), &elem->x) || ImGui::SliderInt(("X##sld" + id_name).c_str(), &elem->x, 0, ul::design::ScreenWidth)) {
                    for(auto &mod_elem: modifiable_elems) {
                        mod_elem->x = elem->x;
                    }
                }
                if(ImGui::InputInt(("Y##inc" + id_name).c_str(), &elem->y) || ImGui::SliderInt(("Y##sld" + id_name).c_str(), &elem->y, 0, ul::design::ScreenHeight)) {
                    for(auto &mod_elem: modifiable_elems) {
                        mod_elem->y = elem->y;
                    }
                }
                if(ImGui::Combo(("Horizontal align##" + id_name).c_str(), &elem->h_align, HorizontalAlignNames, std::size(HorizontalAlignNames))) {
                    for(auto &mod_elem: modifiable_elems) {
                        mod_elem->h_align = elem->h_align;
                    }
                }
                if(ImGui::Combo(("Vertical align##" + id_name).c_str(), &elem->v_align, VerticalAlignNames, std::size(VerticalAlignNames))) {
                    for(auto &mod_elem: modifiable_elems) {
                        mod_elem->v_align = elem->v_align;
                    }
                }
            }
            switch(elem->type) {
                case ul::design::ElementType::Image: {
                    if(!elem->ImageIsBuiltin()) {
                        if(ImGui::Button(("Change image##" + id_name).c_str())) {
                            LoadElementImage(elem);
                        }
                    }
                    break;
                }
                case ul::design::ElementType::Text: {
                    ImGui::Combo(("Font size##" + id_name).c_str(), &elem->el_text.font_size, FontSizeNames, std::size(FontSizeNames));
                    break;
                }
                default:
                    break;
            }
            ImGui::Unindent();
        }
    }

    void MainLoop() {
        const auto cur_width = GetCanvasWidth();
        const auto cur_height = GetCanvasHeight();
        if((cur_width != g_Width) || (cur_height != g_Height)) {
            g_Width = cur_width;
            g_Height = cur_height;
            OnCanvasDimensionsChanged();
        }

        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const auto width = (u32)(g_Width * WindowWidthFactor);
        const auto height = (u32)(g_Height * WindowHeightFactor);

        #define _DRAW_FOR_MENU(name, type, ...) { \
            ImGui::SetNextWindowSize(ImVec2(ul::design::ScreenWidth * g_Scale, ul::design::ScreenHeight * g_Scale + WindowTitleBarExtraHeight), ImGuiCond_Always); \
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0); \
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); \
            if(ImGui::Begin(name)) { \
                if(IsLoadedTheme()) { \
                    elem_background.DrawOnWindow(); \
                    __VA_ARGS__ \
                } \
            } \
            ImGui::End(); \
            ImGui::PopStyleVar(2); \
        }

        _DRAW_FOR_MENU("Main menu", ul::design::MenuType::Main, {
            elem_main_entry_menu_bg.DrawOnWindow();
            elem_main_entry_menu_left_icon.DrawOnWindow();
            elem_main_entry_menu_right_icon.DrawOnWindow();

            switch(g_VisibleTopMenuBackground) {
                case 0: {
                    elem_main_top_menu_default_bg.DrawOnWindow();
                    break;
                }
                case 1: {
                    elem_main_top_menu_app_bg.DrawOnWindow();
                    break;
                }
                case 2: {
                    elem_main_top_menu_hb_bg.DrawOnWindow();
                    break;
                }
                case 3: {
                    elem_main_top_menu_folder_bg.DrawOnWindow();
                    break;
                }
            }

            elem_main_input_bar_bg.DrawOnWindow();
            const auto input_text = "\uE0E0 Demo input   \uE0E1 Demo input   \uE0E2 \uE0E3 Demo input   \uE0E4 \uE0E5 Demo input";
            u32 text_width;
            u32 text_height;
            ImFont *dummy_font;
            float dummy_size;
            ProcessText(input_text, ul::design::FontSize::Small, text_width, text_height, dummy_font, dummy_size);
            DrawTextAt(input_text, ul::design::FontSize::Small, elem_main_input_bar_bg.GetDrawX(ul::design::ScreenWidth) + 37, elem_main_input_bar_bg.GetDrawY(ul::design::ScreenHeight) + (u32)((double)(elem_main_input_bar_bg.el_img.height - text_height) / 2.0f));

            ////// Entry menu

            auto entry_menu_base_x = elem_main_entry_menu.GetDrawX(ul::design::ScreenWidth) + ul::design::EntryMenuHorizontalSideMargin;
            auto entry_menu_base_y = elem_main_entry_menu.GetDrawY(ul::design::ScreenHeight) + ul::design::EntryMenuVerticalSideMargin;

            #define _DRAW_EMPTY_ICON() { \
                elem_main_empty_entry_icon.DrawOnWindowAt(entry_menu_base_x, entry_menu_base_y, ul::design::EntryMenuEntryIconSize, ul::design::EntryMenuEntryIconSize); \
                entry_menu_base_x += ul::design::EntryMenuEntryIconSize + ul::design::EntryMenuEntryMargin; \
            }
            
            #define _DRAW_ICON(elem) { \
                const auto elem##_x = entry_menu_base_x; \
                const auto elem##_y = entry_menu_base_y; \
                elem.DrawOnWindowAt(elem##_x, elem##_y, ul::design::EntryMenuEntryIconSize, ul::design::EntryMenuEntryIconSize); \
                elem_main_border_over_icon.DrawOnWindowAt(elem##_x - (elem_main_border_over_icon.el_img.width - ul::design::EntryMenuEntryIconSize) / 2, elem##_y - (elem_main_border_over_icon.el_img.height - ul::design::EntryMenuEntryIconSize) / 2); \
                entry_menu_base_x += elem.el_img.width + ul::design::EntryMenuEntryMargin; \
            }

            #define _DRAW_ICON_WITH_OVER(elem, over) { \
                const auto elem##_x = entry_menu_base_x; \
                const auto elem##_y = entry_menu_base_y; \
                elem.DrawOnWindowAt(elem##_x, elem##_y, ul::design::EntryMenuEntryIconSize, ul::design::EntryMenuEntryIconSize); \
                elem_main_border_over_icon.DrawOnWindowAt(elem##_x - (elem_main_border_over_icon.el_img.width - elem.el_img.width) / 2, elem##_y - (elem_main_border_over_icon.el_img.height - elem.el_img.height) / 2); \
                entry_menu_base_x += elem.el_img.width + ul::design::EntryMenuEntryMargin; \
                over.DrawOnWindowAt(elem##_x - (over.el_img.width - elem.el_img.width) / 2, elem##_y - (over.el_img.height - elem.el_img.height) / 2); \
            }

            #define _NEXT_ROW() { \
                entry_menu_base_x = elem_main_entry_menu.GetDrawX(ul::design::ScreenWidth) + ul::design::EntryMenuHorizontalSideMargin; \
                entry_menu_base_y += ul::design::EntryMenuEntryIconSize + ul::design::EntryMenuEntryMargin; \
            }

            _DRAW_ICON(elem_main_themes_entry_icon);
            _DRAW_EMPTY_ICON();
            _DRAW_ICON_WITH_OVER(elem_main_settings_entry_icon, elem_main_cursor_over_icon);
            _DRAW_ICON(elem_main_web_browser_entry_icon);
            _DRAW_ICON_WITH_OVER(elem_main_mii_edit_entry_icon, elem_main_selected_over_icon);

            _NEXT_ROW();

            _DRAW_ICON(elem_main_folder_entry_icon);
            _DRAW_ICON_WITH_OVER(elem_main_album_entry_icon, elem_main_suspended_over_icon);
            _DRAW_ICON_WITH_OVER(elem_main_default_app_entry_icon, elem_main_hb_takeover_app_over_icon);
            _DRAW_EMPTY_ICON();
            _DRAW_ICON(elem_main_default_hb_entry_icon);

            /////////////////

            elem_main_logo_top_icon.DrawOnWindow();
            
            switch(g_VisibleConnectionTopIcon) {
                case 0: {
                    elem_main_connection_none_top_icon.DrawOnWindow();
                    break;
                }
                case 1: {
                    elem_main_connection_0_top_icon.DrawOnWindow();
                    break;
                }
                case 2: {
                    elem_main_connection_1_top_icon.DrawOnWindow();
                    break;
                }
                case 3: {
                    elem_main_connection_2_top_icon.DrawOnWindow();
                    break;
                }
                case 4: {
                    elem_main_connection_3_top_icon.DrawOnWindow();
                    break;
                }
            }

            elem_main_time_text.DrawOnWindow();
            elem_main_date_text.DrawOnWindow();
            elem_main_battery_text.DrawOnWindow();
            
            switch(g_VisibleBatteryTopIcon) {
                case 0: {
                    elem_main_battery_10_top_icon.DrawOnWindow();
                    break;
                }
                case 1: {
                    elem_main_battery_20_top_icon.DrawOnWindow();
                    break;
                }
                case 2: {
                    elem_main_battery_30_top_icon.DrawOnWindow();
                    break;
                }
                case 3: {
                    elem_main_battery_40_top_icon.DrawOnWindow();
                    break;
                }
                case 4: {
                    elem_main_battery_50_top_icon.DrawOnWindow();
                    break;
                }
                case 5: {
                    elem_main_battery_60_top_icon.DrawOnWindow();
                    break;
                }
                case 6: {
                    elem_main_battery_70_top_icon.DrawOnWindow();
                    break;
                }
                case 7: {
                    elem_main_battery_80_top_icon.DrawOnWindow();
                    break;
                }
                case 8: {
                    elem_main_battery_90_top_icon.DrawOnWindow();
                    break;
                }
                case 9: {
                    elem_main_battery_100_top_icon.DrawOnWindow();
                    break;
                }
            }

            if(g_DrawBatteryCharging) {
                elem_main_battery_top_icon_charging.DrawOnWindow();
            }

            elem_main_cur_path_text.DrawOnWindow();
            elem_main_cur_entry_main_text.DrawOnWindow();
            elem_main_cur_entry_sub_text.DrawOnWindow();

            if(g_DrawQuickMenu) {
                const std::vector<Element*> quick_menu_icons = {
                    &elem_main_power_quick_icon,
                    &elem_main_controllers_quick_icon,
                    &elem_main_album_quick_icon,
                    &elem_main_web_browser_quick_icon,
                    nullptr,
                    &elem_main_themes_quick_icon,
                    &elem_main_settings_quick_icon,
                    &elem_main_mii_edit_quick_icon
                };
                const std::vector<std::string> quick_menu_names = {
                    "Power options",
                    "Controller options",
                    "Open album",
                    "Open web page",
                    "Open user page",
                    "Open themes menu",
                    "Open settings menu",
                    "Open mii editor"
                };
                DrawItemMenu(quick_menu_icons, quick_menu_names, 1, 300, 172, 1320, 90, IM_COL32(0, 0, 0, 220));
            }
        });

        _DRAW_FOR_MENU("Startup menu", ul::design::MenuType::Startup, {
            elem_startup_info_text.DrawOnWindow();

            const std::vector<Element*> users_menu_icons = {
                nullptr,
                nullptr,
                nullptr
            };
            const std::vector<std::string> users_menu_texts = {
                "User 1",
                "User 2",
                "Add user"
            };
            DrawItemMenuElement(elem_startup_users_menu, users_menu_icons, users_menu_texts, 1, 1320, 150);
        });

        _DRAW_FOR_MENU("Themes menu", ul::design::MenuType::Themes, {
            elem_themes_info_text.DrawOnWindow();

            const std::vector<Element*> themes_menu_icons = {
                &elem_theme_icon,
                nullptr,
                nullptr
            };
            const std::vector<std::string> themes_menu_texts = {
                "Current theme (...)",
                "Demo theme 1 (...)",
                "Reset theme (default theme)"
            };
            DrawItemMenuElement(elem_themes_themes_menu, themes_menu_icons, themes_menu_texts, 1, 1720, 180);
        });

        _DRAW_FOR_MENU("Settings menu", ul::design::MenuType::Settings, {
            elem_settings_info_text.DrawOnWindow();

            const std::vector<Element*> settings_menu_icons = {
                &elem_settings_setting_non_editable_icon,
                &elem_settings_setting_editable_icon,
                &elem_settings_setting_editable_icon
            };
            const std::vector<std::string> settings_menu_texts = {
                "Non-editable setting",
                "Editable setting 1",
                "Editable setting 2"
            };
            DrawItemMenuElement(elem_settings_settings_menu, settings_menu_icons, settings_menu_texts, 1, 1770, 150);
        });

        ImGui::SetNextWindowPos(ImVec2((g_Width - width) / 2, (g_Height - height) / 2), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(750, 800), ImGuiCond_Once);
        if(ImGui::Begin("Theme settings")) {
            ImGui::Text("uDesigner: uLaunch theme viewer and editor");
            if(ImGui::Button("Specification/details about customizable aspects")) {
                OpenUrl("https://github.com/XorTroll/uLaunch/wiki/ThemeSpecs");
            }
            ImGui::NewLine();
            
            ImGui::Text("If any problems occur, check the JavaScript console for any meaningful logs!");

            ImGui::Separator();

            if(ImGui::Button("Load theme")) {
                LoadThemeZip();
            }

            if(ImGui::Button("Load default theme")) {
                LoadDefaultThemeZip();
            }

            if(IsLoadedTheme()) {
                if(ImGui::Button("Save theme")) {
                    SaveThemeZip();
                }

                ImGui::Separator();

                ImGui::InputText("Theme name", g_ManifestName, sizeof(g_ManifestName));
                ImGui::InputText("Theme description", g_ManifestDescription, sizeof(g_ManifestDescription));
                ImGui::InputText("Theme release version", g_ManifestRelease, sizeof(g_ManifestRelease));
                ImGui::InputText("Theme author", g_ManifestAuthor, sizeof(g_ManifestAuthor));

                LoadEditableElement(&elem_theme_icon);

                ImGui::Separator();

                if(ImGui::Button("Restore original")) {
                    ReloadFromTheme();
                }

                ImGui::SliderFloat("Menu preview scale", &g_Scale, 0.1f, 1.0f);

                ImGui::Separator();

                LoadEditableElement(&elem_background);

                ImGui::DragInt("Suspended application final alpha", &g_suspended_app_final_alpha, 1.0f, 0, 255);

                LoadColorPicker("Text color", &g_text_color);
                LoadColorPicker("Menu focus color", &g_menu_focus_color);
                LoadColorPicker("Menu background color", &g_menu_bg_color);
                LoadColorPicker("Dialog title color", &g_dialog_title_color);
                LoadColorPicker("Dialog content color", &g_dialog_cnt_color);
                LoadColorPicker("Dialog option color", &g_dialog_opt_color);
                LoadColorPicker("Dialog color", &g_dialog_color);
                LoadColorPicker("Dialog over color", &g_dialog_over_color);

                ImGui::Separator();

                /*
                if(ImGui::Button("Load custom font")) {
                    LoadFont();
                }
                if(g_FontData != nullptr) {
                    if(ImGui::Button("Reset custom font")) {
                        free(g_FontData);
                        ReloadFonts();
                    }
                }
                */
                ImGui::Text("Font editing: to be implemented (must be added manually)");

                ImGui::Text("BGM and sound effect editing: to be implemented (must be added manually)");

                ImGui::Separator();

                if(ImGui::CollapsingHeader("Main menu")) {
                    ImGui::Indent();

                    ImGui::Combo("Previewed top menu", &g_VisibleTopMenuBackground, TopMenuBackgroundNames, std::size(TopMenuBackgroundNames));
                    ImGui::Combo("Previewed connection top icon", &g_VisibleConnectionTopIcon, ConnectionTopIconNames, std::size(ConnectionTopIconNames));
                    ImGui::Combo("Previewed battery top icon", &g_VisibleBatteryTopIcon, BatteryTopIconNames, std::size(BatteryTopIconNames));

                    ImGui::Checkbox("Preview battery charging", &g_DrawBatteryCharging);
                    ImGui::Checkbox("Preview quick menu", &g_DrawQuickMenu);

                    ImGui::Separator();

                    switch(g_VisibleTopMenuBackground) {
                        case 0: {
                            LoadEditableElement(&elem_main_top_menu_default_bg, {
                                &elem_main_top_menu_app_bg,
                                &elem_main_top_menu_hb_bg,
                                &elem_main_top_menu_folder_bg
                            });
                            break;
                        }
                        case 1: {
                            LoadEditableElement(&elem_main_top_menu_app_bg, {
                                &elem_main_top_menu_default_bg,
                                &elem_main_top_menu_hb_bg,
                                &elem_main_top_menu_folder_bg
                            });
                            break;
                        }
                        case 2: {
                            LoadEditableElement(&elem_main_top_menu_hb_bg, {
                                &elem_main_top_menu_default_bg,
                                &elem_main_top_menu_app_bg,
                                &elem_main_top_menu_folder_bg
                            });
                            break;
                        }
                        case 3: {
                            LoadEditableElement(&elem_main_top_menu_folder_bg, {
                                &elem_main_top_menu_default_bg,
                                &elem_main_top_menu_app_bg,
                                &elem_main_top_menu_hb_bg
                            });
                            break;
                        }
                    }

                    LoadEditableElement(&elem_main_logo_top_icon);

                    switch(g_VisibleConnectionTopIcon) {
                        case 0: {
                            LoadEditableElement(&elem_main_connection_none_top_icon, {
                                &elem_main_connection_0_top_icon,
                                &elem_main_connection_1_top_icon, 
                                &elem_main_connection_2_top_icon, 
                                &elem_main_connection_3_top_icon
                            });
                            break;
                        }
                        case 1: {
                            LoadEditableElement(&elem_main_connection_0_top_icon, {
                                &elem_main_connection_1_top_icon, 
                                &elem_main_connection_2_top_icon, 
                                &elem_main_connection_3_top_icon,
                                &elem_main_connection_none_top_icon
                            });
                            break;
                        }
                        case 2: {
                            LoadEditableElement(&elem_main_connection_1_top_icon, {
                                &elem_main_connection_2_top_icon, 
                                &elem_main_connection_3_top_icon,
                                &elem_main_connection_none_top_icon,
                                &elem_main_connection_0_top_icon
                            });
                            break;
                        }
                        case 3: {
                            LoadEditableElement(&elem_main_connection_2_top_icon, {
                                &elem_main_connection_3_top_icon,
                                &elem_main_connection_none_top_icon,
                                &elem_main_connection_0_top_icon, 
                                &elem_main_connection_1_top_icon
                            });
                            break;
                        }
                        case 4: {
                            LoadEditableElement(&elem_main_connection_3_top_icon, {
                                &elem_main_connection_none_top_icon,
                                &elem_main_connection_0_top_icon, 
                                &elem_main_connection_1_top_icon, 
                                &elem_main_connection_2_top_icon
                            });
                            break;
                        }
                    }

                    LoadEditableElement(&elem_main_time_text);
                    LoadEditableElement(&elem_main_date_text);
                    LoadEditableElement(&elem_main_battery_text);

                    switch(g_VisibleBatteryTopIcon) {
                        case 0: {
                            LoadEditableElement(&elem_main_battery_10_top_icon, {
                                &elem_main_battery_20_top_icon,
                                &elem_main_battery_30_top_icon,
                                &elem_main_battery_40_top_icon,
                                &elem_main_battery_50_top_icon,
                                &elem_main_battery_60_top_icon,
                                &elem_main_battery_70_top_icon,
                                &elem_main_battery_80_top_icon,
                                &elem_main_battery_90_top_icon,
                                &elem_main_battery_100_top_icon,
                                &elem_main_battery_top_icon_charging
                            });
                            break;
                        }
                        case 1: {
                            LoadEditableElement(&elem_main_battery_20_top_icon, {
                                &elem_main_battery_30_top_icon,
                                &elem_main_battery_40_top_icon,
                                &elem_main_battery_50_top_icon,
                                &elem_main_battery_60_top_icon,
                                &elem_main_battery_70_top_icon,
                                &elem_main_battery_80_top_icon,
                                &elem_main_battery_90_top_icon,
                                &elem_main_battery_100_top_icon,
                                &elem_main_battery_top_icon_charging,
                                &elem_main_battery_10_top_icon
                            });
                            break;
                        }
                        case 2: {
                            LoadEditableElement(&elem_main_battery_30_top_icon, {
                                &elem_main_battery_40_top_icon,
                                &elem_main_battery_50_top_icon,
                                &elem_main_battery_60_top_icon,
                                &elem_main_battery_70_top_icon,
                                &elem_main_battery_80_top_icon,
                                &elem_main_battery_90_top_icon,
                                &elem_main_battery_100_top_icon,
                                &elem_main_battery_top_icon_charging,
                                &elem_main_battery_10_top_icon,
                                &elem_main_battery_20_top_icon
                            });
                            break;
                        }
                        case 3: {
                            LoadEditableElement(&elem_main_battery_40_top_icon, {
                                &elem_main_battery_50_top_icon,
                                &elem_main_battery_60_top_icon,
                                &elem_main_battery_70_top_icon,
                                &elem_main_battery_80_top_icon,
                                &elem_main_battery_90_top_icon,
                                &elem_main_battery_100_top_icon,
                                &elem_main_battery_top_icon_charging,
                                &elem_main_battery_10_top_icon,
                                &elem_main_battery_20_top_icon,
                                &elem_main_battery_30_top_icon
                            });
                            break;
                        }
                        case 4: {
                            LoadEditableElement(&elem_main_battery_50_top_icon, {
                                &elem_main_battery_60_top_icon,
                                &elem_main_battery_70_top_icon,
                                &elem_main_battery_80_top_icon,
                                &elem_main_battery_90_top_icon,
                                &elem_main_battery_100_top_icon,
                                &elem_main_battery_top_icon_charging,
                                &elem_main_battery_10_top_icon,
                                &elem_main_battery_20_top_icon,
                                &elem_main_battery_30_top_icon,
                                &elem_main_battery_40_top_icon
                            });
                            break;
                        }
                        case 5: {
                            LoadEditableElement(&elem_main_battery_60_top_icon, {
                                &elem_main_battery_70_top_icon,
                                &elem_main_battery_80_top_icon,
                                &elem_main_battery_90_top_icon,
                                &elem_main_battery_100_top_icon,
                                &elem_main_battery_top_icon_charging,
                                &elem_main_battery_10_top_icon,
                                &elem_main_battery_20_top_icon,
                                &elem_main_battery_30_top_icon,
                                &elem_main_battery_40_top_icon,
                                &elem_main_battery_50_top_icon
                            });
                            break;
                        }
                        case 6: {
                            LoadEditableElement(&elem_main_battery_70_top_icon, {
                                &elem_main_battery_80_top_icon,
                                &elem_main_battery_90_top_icon,
                                &elem_main_battery_100_top_icon,
                                &elem_main_battery_top_icon_charging,
                                &elem_main_battery_10_top_icon,
                                &elem_main_battery_20_top_icon,
                                &elem_main_battery_30_top_icon,
                                &elem_main_battery_40_top_icon,
                                &elem_main_battery_50_top_icon,
                                &elem_main_battery_60_top_icon
                            });
                            break;
                        }
                        case 7: {
                            LoadEditableElement(&elem_main_battery_80_top_icon, {
                                &elem_main_battery_90_top_icon,
                                &elem_main_battery_100_top_icon,
                                &elem_main_battery_top_icon_charging,
                                &elem_main_battery_10_top_icon,
                                &elem_main_battery_20_top_icon,
                                &elem_main_battery_30_top_icon,
                                &elem_main_battery_40_top_icon,
                                &elem_main_battery_50_top_icon,
                                &elem_main_battery_60_top_icon,
                                &elem_main_battery_70_top_icon
                            });
                            break;
                        }
                        case 8: {
                            LoadEditableElement(&elem_main_battery_90_top_icon, {
                                &elem_main_battery_100_top_icon,
                                &elem_main_battery_top_icon_charging,
                                &elem_main_battery_10_top_icon,
                                &elem_main_battery_20_top_icon,
                                &elem_main_battery_30_top_icon,
                                &elem_main_battery_40_top_icon,
                                &elem_main_battery_50_top_icon,
                                &elem_main_battery_60_top_icon,
                                &elem_main_battery_70_top_icon,
                                &elem_main_battery_80_top_icon
                            });
                            break;
                        }
                        case 9: {
                            LoadEditableElement(&elem_main_battery_100_top_icon, {
                                &elem_main_battery_top_icon_charging,
                                &elem_main_battery_10_top_icon,
                                &elem_main_battery_20_top_icon,
                                &elem_main_battery_30_top_icon,
                                &elem_main_battery_40_top_icon,
                                &elem_main_battery_50_top_icon,
                                &elem_main_battery_60_top_icon,
                                &elem_main_battery_70_top_icon,
                                &elem_main_battery_80_top_icon,
                                &elem_main_battery_90_top_icon
                            });
                            break;
                        }
                    }

                    LoadEditableElement(&elem_main_input_bar_bg);

                    LoadEditableElement(&elem_main_cur_path_text);
                    LoadEditableElement(&elem_main_cur_entry_main_text);
                    LoadEditableElement(&elem_main_cur_entry_sub_text);
                    LoadEditableElement(&elem_main_entry_menu_bg);
                    LoadEditableElement(&elem_main_entry_menu_left_icon);
                    LoadEditableElement(&elem_main_entry_menu_right_icon);

                    LoadEditableElement(&elem_main_entry_menu);

                    LoadEditableElement(&elem_main_album_entry_icon);
                    LoadEditableElement(&elem_main_controllers_entry_icon);
                    LoadEditableElement(&elem_main_default_app_entry_icon);
                    LoadEditableElement(&elem_main_default_hb_entry_icon);
                    LoadEditableElement(&elem_main_empty_entry_icon);
                    LoadEditableElement(&elem_main_folder_entry_icon);
                    LoadEditableElement(&elem_main_mii_edit_entry_icon);
                    LoadEditableElement(&elem_main_settings_entry_icon);
                    LoadEditableElement(&elem_main_themes_entry_icon);
                    LoadEditableElement(&elem_main_web_browser_entry_icon);

                    LoadEditableElement(&elem_main_album_quick_icon);
                    LoadEditableElement(&elem_main_controllers_quick_icon);
                    LoadEditableElement(&elem_main_mii_edit_quick_icon);
                    LoadEditableElement(&elem_main_power_quick_icon);
                    LoadEditableElement(&elem_main_settings_quick_icon);
                    LoadEditableElement(&elem_main_themes_quick_icon);
                    LoadEditableElement(&elem_main_web_browser_quick_icon);

                    LoadEditableElement(&elem_main_border_over_icon);
                    LoadEditableElement(&elem_main_cursor_over_icon);
                    LoadEditableElement(&elem_main_hb_takeover_app_over_icon);
                    LoadEditableElement(&elem_main_selected_over_icon);
                    LoadEditableElement(&elem_main_suspended_over_icon);
                    
                    ImGui::Unindent();
                }

                if(ImGui::CollapsingHeader("Startup menu")) {
                    ImGui::Indent();

                    LoadEditableElement(&elem_startup_info_text);
                    LoadEditableElement(&elem_startup_users_menu);
                    
                    ImGui::Unindent();
                }

                if(ImGui::CollapsingHeader("Themes menu")) {
                    ImGui::Indent();

                    LoadEditableElement(&elem_themes_info_text);
                    LoadEditableElement(&elem_themes_themes_menu);
                    
                    ImGui::Unindent();
                }

                if(ImGui::CollapsingHeader("Settings menu")) {
                    ImGui::Indent();

                    LoadEditableElement(&elem_settings_info_text);
                    LoadEditableElement(&elem_settings_settings_menu);
                    LoadEditableElement(&elem_settings_setting_editable_icon);
                    LoadEditableElement(&elem_settings_setting_non_editable_icon);
                    
                    ImGui::Unindent();
                }
            }

            ImGui::End();
        }

        ImGui::Render();

        glfwMakeContextCurrent(g_Window);
        int display_w, display_h;
        glfwGetFramebufferSize(g_Window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwMakeContextCurrent(g_Window);
    }

    int InitializeGlfw() {
        if(glfwInit() != GLFW_TRUE) {
            ShowError("Failed to initialize GLFW!");
            return 1;
        }

        // We don't want the old OpenGL
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Open a window and create its OpenGL context
        g_Window = glfwCreateWindow(g_Width, g_Height, "WebGui Demo", nullptr, nullptr);
        if(g_Window == nullptr) {
            ShowError("Failed to open GLFW window!");
            glfwTerminate();
            return -1;
        }

        // Initialize GLFW
        glfwMakeContextCurrent(g_Window);
        return 0;
    }

    int InitializeImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
        ImGui_ImplOpenGL3_Init();

        ImGui::StyleColorsDark();

        auto &io = ImGui::GetIO();

        io.Fonts->AddFontDefault();

        #define _LOAD_BUILTIN_FONT_FOR_SIZE(name, size) { \
            ImFontConfig config; \
            config.MergeMode = true; \
            g_FontStandard_##name = io.Fonts->AddFontFromFileTTF("assets/FontStandard.ttf", size * g_Scale, nullptr, StandardGlyphRanges); \
            g_FontExtended_##name = io.Fonts->AddFontFromFileTTF("assets/FontExtended.ttf", size * g_Scale, &config, ExtendedGlyphRanges); \
        }
        _LOAD_BUILTIN_FONT_FOR_SIZE(Small, 27.0f)
        _LOAD_BUILTIN_FONT_FOR_SIZE(Medium, 30.0f)
        _LOAD_BUILTIN_FONT_FOR_SIZE(MediumLarge, 37.0f)
        _LOAD_BUILTIN_FONT_FOR_SIZE(Large, 45.0f)

        io.Fonts->Build();

        ResizeCanvas();

        return 0;
    }

    #define _RES_TRY(expr) { \
        const auto tmp_res = (expr); \
        if(tmp_res != 0) { \
            return tmp_res; \
        } \
    }

    int Initialize() {
        srand(time(nullptr));

        g_Width = GetCanvasWidth();
        g_Height = GetCanvasHeight();

        _RES_TRY(InitializeGlfw());
        _RES_TRY(InitializeImGui());

        return 0;
    }

    void Finalize() {
        glfwTerminate();
    }

}

extern "C" int main(int argc, char **argv) {
    if(Initialize() != 0) {
        ShowError("Failed to initialize!");
        return 1;
    }

    emscripten_set_main_loop(MainLoop, 0, 1);

    Finalize();
    return 0;
}
