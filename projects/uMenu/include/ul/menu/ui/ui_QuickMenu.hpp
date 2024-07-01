
#pragma once
#include <pu/Plutonium>
#include <ul/menu/smi/smi_MenuMessageHandler.hpp>

namespace ul::menu::ui {

    class QuickMenu : public pu::ui::elm::Element {
        public:
            static constexpr u8 BackgroundAlphaMax = 0xDC;
            static constexpr u8 BackgroundAlphaIncrement = 20;

            static constexpr u32 MenuMargin = 300;
            static constexpr u32 MenuX = MenuMargin;
            static constexpr u32 MenuY = 172;
            static constexpr u32 MenuWidth = pu::ui::render::ScreenWidth - 2 * MenuMargin;
            static constexpr u32 MenuItemHeight = 90;
            static constexpr u32 MenuItemsToShow = 8;

            static inline constexpr pu::ui::Color MakeBackgroundColor(const u8 alpha) {
                return { 50, 50, 50, alpha };
            }

        private:
            bool on;
            s32 bg_alpha;
            pu::ui::elm::Menu::Ref options_menu;
            pu::ui::elm::MenuItem::Ref power_menu_item;
            pu::ui::elm::MenuItem::Ref controller_menu_item;
            pu::ui::elm::MenuItem::Ref album_menu_item;
            pu::ui::elm::MenuItem::Ref web_menu_item;
            pu::ui::elm::MenuItem::Ref user_menu_item;
            pu::ui::elm::MenuItem::Ref themes_menu_item;
            pu::ui::elm::MenuItem::Ref settings_menu_item;
            pu::ui::elm::MenuItem::Ref mii_menu_item;

            static void OnHomeButtonDetection(const smi::MenuMessageContext _);

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

            void UpdateItems();

            void OnRender(pu::ui::render::Renderer::Ref &drawer, const s32 x, const s32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;

            static inline void RegisterHomeButtonDetection() {
                smi::RegisterOnMessageDetect(&OnHomeButtonDetection, smi::MenuMessage::HomeRequest);
            }
    };

}
