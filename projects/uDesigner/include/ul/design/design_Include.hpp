
#pragma once
#include <cstdint>

#define GLFW_INCLUDE_ES3
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#ifndef __EMSCRIPTEN__
#error "This should be compiled as JS!"
#endif
#include <emscripten.h>

#include <json.hpp>
#include <src/zip.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

namespace ul::design {

    enum class MenuType {
        None,
        Main,
        Startup,
        Themes,
        Settings,
        Lockscreen
    };

    constexpr const char *MenuSettingsNames[] = {
        "",
        "main_menu",
        "startup_menu",
        "themes_menu",
        "settings_menu",
        "lockscreen_menu"
    };

    enum class ElementType {
        None,
        Image,
        Text
    };

    enum class HorizontalAlign {
        Left,
        Center,
        Right
    };

    enum class VerticalAlign {
        Top,
        Center,
        Bottom
    };

    enum class FontSize {
        Small,
        Medium,
        MediumLarge,
        Large
    };

    constexpr float FontSizeSmall = 27.0f;
    constexpr float FontSizeMedium = 30.0f;
    constexpr float FontSizeMediumLarge = 37.0f;
    constexpr float FontSizeLarge = 45.0f;

    constexpr u32 EntryMenuHorizontalSideMargin = 60;
    constexpr u32 EntryMenuVerticalSideMargin = 35;
    constexpr u32 EntryMenuEntryMargin = 35;
    constexpr u32 EntryMenuEntryIconSize = 384;

    constexpr auto ManifestPath = "theme/Manifest.json";
    constexpr auto UiSettingsPath = "ui/UI.json";
    constexpr auto SoundSettingsPath = "sound/BGM.json";

    constexpr u32 CurrentFormatVersion = 3;

    constexpr u32 ScreenWidth = 1920;
    constexpr u32 ScreenHeight = 1080;

    constexpr u32 ItemMenuDefaultIconMargin = 37;
    constexpr u32 ItemMenuDefaultTextMargin = 37;

}
