
#pragma once
#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/menu/ui/ui_InputBar.hpp>
#include <ul/cfg/cfg_Config.hpp>

namespace ul::menu::ui {

    enum class SettingMenu {
        System,
        uLaunch,
        Bluetooth,
        Network,
        Screen,
        Dev,

        Count
    };

    enum class SettingSubmenu {
        None
    };

    enum class Setting {
        ConsoleFirmware,
        AtmosphereFirmware,
        AtmosphereEmummc,
        ConsoleNickname,
        ConsoleTimezone,
        UsbScreenCaptureEnabled,
        ConnectionSsid,
        ConsoleLanguage,
        ConsoleInformationUploadEnabled,
        AutomaticApplicationDownloadEnabled,
        ConsoleAutoUpdateEnabled,
        WirelessLanEnabled,
        BluetoothEnabled,
        Usb30Enabled,
        NfcEnabled,
        ConsoleSerialNumber,
        MacAddress,
        IpAddress,

        ConsoleRegion,
        SystemUpdateStatus,
        LockscreenEnabled,
        PrimaryAlbumStorage,
        HandheldSleepPlan,
        ConsoleSleepPlan,
        SleepWhilePlayingMedia,
        SleepWakesAtPowerStateChange,
        BatteryLot,

        uLaunchVersion,
        AudioService,
    };

    class SettingsMenuLayout : public IMenuLayout {
        public:
            static constexpr u32 SettingsMenuWidth = 1770;
            static constexpr u32 SettingsMenuItemSize = 150;
            static constexpr u32 SettingsMenuItemsToShow = 5;

        private:
            pu::ui::elm::TextBlock::Ref menu_text;
            pu::ui::elm::TextBlock::Ref submenu_text;
            pu::ui::elm::Menu::Ref settings_menu;
            InputBar::Ref input_bar;
            bool inputs_changed;
            pu::audio::Sfx setting_edit_sfx;
            pu::audio::Sfx setting_save_sfx;
            pu::audio::Sfx back_sfx;
            pu::audio::Sfx setting_menu_move_sfx;
            SettingMenu cur_menu;
            SettingSubmenu cur_submenu;

            void PushSettingItem(const std::string &name, const std::string &value_display, const int id);
            void setting_DefaultKey(const u32 id);

        public:
            SettingsMenuLayout();
            PU_SMART_CTOR(SettingsMenuLayout)

            void OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) override;
            bool OnHomeButtonPress() override;
            void LoadSfx() override;
            void DisposeSfx() override;

            void Reload(const bool soft);
    };

}
