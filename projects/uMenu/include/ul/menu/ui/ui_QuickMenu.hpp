
#pragma once
#include <ul/menu/smi/smi_SystemMessageHandler.hpp>
#include <pu/Plutonium>

namespace ul::menu::ui {

    class QuickMenu : public pu::ui::elm::Element {
        public:
            static constexpr s32 MainItemSize = 300;
            static constexpr s32 SubItemsSize = 150;
            static constexpr s32 CommonAreaSize = 50;

            static constexpr s32 MainItemX = (pu::ui::render::ScreenWidth - MainItemSize) / 2;
            static constexpr s32 MainItemY = (pu::ui::render::ScreenHeight - MainItemSize) / 2;

            static constexpr u8 BackgroundAlphaMax = 0xDC;
            static constexpr u8 BackgroundAlphaIncrement = 20;

            static constexpr u32 MenuMargin = 200;
            static constexpr u32 MenuX = MenuMargin;
            static constexpr u32 MenuY = 115;
            static constexpr u32 MenuWidth = pu::ui::render::ScreenWidth - 2 * MenuMargin;
            static constexpr u32 MenuItemHeight = 60;
            static constexpr u32 MenuItemsToShow = 8;

            static inline constexpr pu::ui::Color MakeBackgroundColor(const u8 alpha) {
                return { 50, 50, 50, alpha };
            }

        private:
            bool on;
            s32 bg_alpha;
            pu::ui::elm::Menu::Ref options_menu;

            static void OnHomeButtonDetection();

        public:
            QuickMenu();
            PU_SMART_CTOR(QuickMenu)

            inline constexpr s32 GetX() override {
                return 0;
            }

            inline constexpr s32 GetY() override {
                return 0;
            }

            inline constexpr s32 GetWidth() override {
                return pu::ui::render::ScreenWidth;
            }

            inline constexpr s32 GetHeight() override {
                return pu::ui::render::ScreenHeight;
            }
            
            inline void Toggle() {
                this->on = !this->on;
            }

            inline constexpr bool IsOn() {
                return this->on && (this->bg_alpha > 0);
            }

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;

            static inline void RegisterHomeButtonDetection() {
                smi::RegisterOnMessageDetect(&OnHomeButtonDetection, smi::MenuMessage::HomeRequest);
            }
    };

}