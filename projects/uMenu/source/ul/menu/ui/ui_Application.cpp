#include <ul/menu/ui/ui_Application.hpp>
#include <ul/menu/ui/ui_Util.hpp>
// #include <ul/menu/thm/api/api_Api.hpp>

extern ul::menu::ui::Application::Ref g_Application;
extern ul::smi::SystemStatus g_SystemStatus;

namespace ul::menu::ui {

    namespace {

        inline bool IsAnySuspended() {
            return (g_SystemStatus.suspended_app_id > 0) || g_SystemStatus.suspended_hb_target_ipt.IsValid();
        }

    }

    void UiOnHomeButtonDetection() {
        g_Application->GetLayout<menu::MenuLayout>()->DoOnHomeButtonPress();
    }

    void UiOnSdCardEjectedDetection() {
        g_Application->CreateShowDialog("SD ejected", "close this dialog to reboot", { "ok" }, true);
        Reboot();
    }

    void Application::OnLoad() {
        const auto toast_text_clr = pu::ui::Color { 0xe1, 0xe1, 0xe1, 0xff };
        const auto toast_base_clr = pu::ui::Color { 0x28, 0x28, 0x28, 0xff };
        this->notif_toast = pu::ui::extras::Toast::New("...", pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium), toast_text_clr, toast_base_clr);

        u8 *screen_capture_buf = nullptr;
        if(IsAnySuspended()) {
            constexpr auto buf_size = 1280 * 720 * 4;
            screen_capture_buf = new u8[buf_size]();

            bool dummy_flag;
            UL_RC_ASSERT(appletGetLastForegroundCaptureImageEx(screen_capture_buf, buf_size, &dummy_flag));
        }

        this->main_menu_lyt = menu::MainMenuLayout::New(screen_capture_buf, 80);
        this->settings_menu_lyt = menu::SettingsMenuLayout::New();
        /*
        thm::api::DebugLog("Pre load api");
        auto res = thm::api::LoadApi(thm::MenuId::MainMenu, this->main_menu_lyt);
        thm::api::DebugLog("Post load api");
        if(res) {
            // fatalThrow(0xbabe);
            thm::api::DebugLog("NICE");
        }
        else {
            fatalThrow(0xbeef);
        }
        */

        this->LoadMainMenu();

        // thm::api::GetCurrentApiContext().OnEvent(thm::api::EventType::Load);
    }

    void Application::ShowNotification(const std::string &text, const u64 timeout) {
        this->EndOverlay();
        this->notif_toast->SetText(text);
        this->StartOverlayWithTimeout(this->notif_toast, timeout);
    }

}