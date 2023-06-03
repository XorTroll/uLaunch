#include <ul/menu/ui/menu/menu_MainMenuLayout.hpp>
#include <ul/menu/ui/ui_Application.hpp>
#include <ul/menu/ui/ui_TransitionGuard.hpp>
#include <ul/menu/ui/ui_Util.hpp>
#include <ul/menu/smi/smi_Commands.hpp>
// #include <ul/menu/thm/api/api_Api.hpp>

extern ul::menu::ui::Application::Ref g_Application;
extern ul::menu::ui::TransitionGuard g_TransitionGuard;
extern ul::smi::SystemStatus g_SystemStatus;
extern ul::smi::MenuStartMode g_MenuStartMode;

namespace ul::menu::ui::menu {

    namespace {

        std::string GetCurrentTime() {
            const auto time_val = std::time(nullptr);
            const auto local_time = std::localtime(&time_val);
            const auto h = local_time->tm_hour;
            const auto min = local_time->tm_min;

            char str[0x10] = {};
            std::sprintf(str, "%02d:%02d", h, min);
            return str;
        }

    }

    MainMenuLayout::MainMenuLayout(const u8 *captured_screen_buf, const u8 min_alpha) : cur_has_connection(false), cur_battery_lvl(0), cur_is_charging(false), captured_screen_buf(captured_screen_buf), launch_failure_warn_shown(false), min_alpha(min_alpha), mode(0), goal_reach_cb(), suspended_screen_alpha_goal(SuspendedScreenAlphaNoneGoal), suspended_screen_alpha(0xFF), status(InputStatus::Normal) {
        this->top_menu_img = pu::ui::elm::Image::New(40, 20, "sdmc:/umad/uitest/top_menu_bg.png");
        this->Add(this->top_menu_img);

        this->logo_img = ClickableImage::New(610, 13 + 20, "sdmc:/umad/uitest/logo.png");
        this->logo_img->SetWidth(60);
        this->logo_img->SetHeight(60);
        this->logo_img->SetOnClick(&ShowAboutDialog);
        this->Add(this->logo_img);

        this->connection_icon = pu::ui::elm::Image::New(80, 37, "sdmc:/umad/uitest/no_conn.png");
        this->Add(this->connection_icon);

        this->users_img = ClickableImage::New(200, 37, "sdmc:/umad/uitest/user.png");
        this->users_img->SetOnClick(&ShowUserMenu);
        this->Add(this->users_img);

        this->controller_img = ClickableImage::New(270, 37, "sdmc:/umad/uitest/controller.png");
        this->controller_img->SetOnClick(&ShowControllerSupport);
        this->Add(this->controller_img);

        this->mii_img = ClickableImage::New(340, 37, "sdmc:/umad/uitest/mii.png");
        this->mii_img->SetOnClick(&ShowMiiEdit);
        this->Add(this->mii_img);

        this->amiibo_img = ClickableImage::New(410, 37, "sdmc:/umad/uitest/amiibo.png");
        this->amiibo_img->SetOnClick(&ShowAmiiboSettings);
        this->Add(this->amiibo_img);

        const pu::ui::Color text_clr = { 0xff, 0xff, 0xff, 0xff };

        this->time_text = pu::ui::elm::TextBlock::New(515, 48, "...");
        this->time_text->SetColor(text_clr);
        this->Add(this->time_text);

        this->battery_text = pu::ui::elm::TextBlock::New(700, 37, "...");
        this->battery_text->SetFont(pu::ui::GetDefaultFont(pu::ui::DefaultFontSize::Medium));
        this->battery_text->SetColor(text_clr);
        this->Add(this->battery_text);

        this->battery_icon = pu::ui::elm::Image::New(700, 60, "sdmc:/umad/uitest/battery_normal.png");
        this->Add(this->battery_icon);

        this->settings_img = ClickableImage::New(810, 37, "sdmc:/umad/uitest/settings.png");
        this->settings_img->SetOnClick(&ShowSettingsMenu);
        this->Add(this->settings_img);

        this->themes_img = ClickableImage::New(880, 37, "sdmc:/umad/uitest/themes.png");
        this->themes_img->SetOnClick(&ShowThemesMenu);
        this->Add(this->themes_img);

        this->fw_text = pu::ui::elm::TextBlock::New(970, 48, g_SystemStatus.fw_version);
        this->fw_text->SetColor(text_clr);
        this->Add(this->fw_text);

        this->entry_menu = EntryMenu::New(125, pu::ui::render::ScreenHeight - 125 - 50);
        this->Add(this->entry_menu);

        this->input_bar = InputBar::New(pu::ui::render::ScreenHeight - 50);
        this->UpdateInput(this->status);
        this->Add(this->input_bar);

        this->quick_menu = QuickMenu::New();
        this->Add(this->quick_menu);

        if(captured_screen_buf != nullptr) {
            this->suspended_screen_img = RawRgbaImage::New(0, 0, captured_screen_buf, 1280, 720, 4);
            this->Add(this->suspended_screen_img);
        }
        else {
            this->suspended_screen_img = {};
        }

        this->startup_tp = std::chrono::steady_clock::now();

        this->SetBackgroundImage("sdmc:/umad/uitest/bg.png");
    }

    MainMenuLayout::~MainMenuLayout() {
        if(this->captured_screen_buf != nullptr) {
            delete[] this->captured_screen_buf;
            this->captured_screen_buf = nullptr;
        }
    }

    void MainMenuLayout::UpdateInput(const InputStatus status) {
        const auto old_status = this->status;
        const auto is_in_folder = static_cast<bool>(status & InputStatus::InFolder);
        const auto is_cur_entry_invalid = static_cast<bool>(status & InputStatus::CurrentEntryInvalid);
        const auto is_cur_entry_folder = static_cast<bool>(status & InputStatus::CurrentEntryFolder);
        const auto is_cur_entry_suspended = static_cast<bool>(status & InputStatus::CurrentEntrySuspended);
        const auto is_entry_selected = static_cast<bool>(status & InputStatus::EntrySelected);

        if(is_cur_entry_invalid) {
            this->input_bar->AddSetInput(HidNpadButton_A, "");
        }
        else if(is_cur_entry_suspended) {
            this->input_bar->AddSetInput(HidNpadButton_A, "Resume");
        }
        else if(is_cur_entry_folder) {
            this->input_bar->AddSetInput(HidNpadButton_A, "Open");
        }
        else {
            this->input_bar->AddSetInput(HidNpadButton_A, "Launch");
        }

        this->input_bar->AddSetInput(HidNpadButton_B, is_in_folder ? "Back" : "");
        this->input_bar->AddSetInput(HidNpadButton_X, is_entry_selected ? "Cancel selection" : (is_cur_entry_suspended ? "Close" : ""));
        this->input_bar->AddSetInput(HidNpadButton_Y, is_entry_selected ? "Move selection" : (is_cur_entry_invalid ? "" : "Select"));
        this->input_bar->AddSetInput(HidNpadButton_StickL, is_cur_entry_invalid ? "" : "Entry options");
        this->input_bar->AddSetInput(HidNpadButton_StickR, "Create folder");
        this->input_bar->AddSetInput(HidNpadButton_Plus | HidNpadButton_Minus, "Resize grid");
        this->input_bar->AddSetInput(HidNpadButton_L | HidNpadButton_R, "Help & about");
        this->input_bar->AddSetInput(HidNpadButton_ZL | HidNpadButton_ZR, "Quick menu");

        this->status = status;

        if(this->suspended_screen_img) {
            const auto old_is_cur_entry_suspended = static_cast<bool>(old_status & InputStatus::CurrentEntrySuspended);
            if(is_cur_entry_suspended != old_is_cur_entry_suspended) {
                if(old_is_cur_entry_suspended) {
                    this->suspended_screen_alpha_goal = SuspendedScreenAlphaNoneGoal;
                }
                else {
                    this->suspended_screen_alpha_goal = SuspendedScreenAlphaSuspendedGoal;
                }
            }
        }
    }

    void MainMenuLayout::NotifyResume() {
        this->goal_reach_cb = []() {
            UL_RC_ASSERT(smi::ResumeApplication());
        };

        this->suspended_screen_alpha_goal = SuspendedScreenAlphaFullGoal;
    }

    void MainMenuLayout::OnMenuInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        // thm::api::GetCurrentApiContext().OnEvent(thm::api::EventType::Input, keys_down, keys_up, keys_held);

        if(this->suspended_screen_img) {
            if(this->suspended_screen_alpha_goal == static_cast<u8>(this->suspended_screen_alpha)) {
                if(this->goal_reach_cb) {
                    this->goal_reach_cb();
                    this->goal_reach_cb = {};
                }
            }

            this->VariateSuspendedScreenAlphaTo(this->suspended_screen_alpha_goal);
            this->suspended_screen_img->SetAlpha(static_cast<u8>(this->suspended_screen_alpha));
        }

        const auto quick_menu_on = this->quick_menu->IsOn();
        this->entry_menu->SetEnabled(!quick_menu_on);
        if(quick_menu_on) {
            this->suspended_screen_alpha_goal = SuspendedScreenAlphaNoneGoal;
            return;
        }

        auto conn_status = NifmInternetConnectionStatus_ConnectingUnknown1;
        // Not checking the result since it will fail if there is no Internet connection
        nifmGetInternetConnectionStatus(nullptr, nullptr, &conn_status);
        const auto has_conn = conn_status == NifmInternetConnectionStatus_Connected;
        if(this->cur_has_connection != has_conn) {
            this->cur_has_connection = has_conn;
            const std::string conn_img = has_conn ? "conn.png" : "no_conn.png";
            this->connection_icon->SetImage("sdmc:/umad/uitest/" + conn_img);
        }

        const auto cur_time = GetCurrentTime();
        this->time_text->SetText(cur_time);

        u32 battery_lvl;
        UL_RC_ASSERT(psmGetBatteryChargePercentage(&battery_lvl));
        if(this->cur_battery_lvl != battery_lvl) {
            this->cur_battery_lvl = battery_lvl;
            const auto battery_str = std::to_string(battery_lvl) + "%";
            this->battery_text->SetText(battery_str);
        }

        PsmChargerType charger_type;
        UL_RC_ASSERT(psmGetChargerType(&charger_type));
        const auto is_charging = charger_type != PsmChargerType_Unconnected;
        if(this->cur_is_charging != is_charging) {
            this->cur_is_charging = is_charging;
            const std::string battery_img = is_charging ? "battery_charging.png" : "battery_normal.png";
            this->battery_icon->SetImage("sdmc:/umad/uitest/" + battery_img);
        }

        const auto now_tp = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now_tp - this->startup_tp).count() >= 500) {
            if(!this->launch_failure_warn_shown) {
                if(g_MenuStartMode == ul::smi::MenuStartMode::MenuLaunchFailure) {
                    g_Application->CreateShowDialog("App launch", "Unexpected error", { "Ok" }, true);
                }
                this->launch_failure_warn_shown = true;
            }
        }
    }

    bool MainMenuLayout::OnHomeButtonPress() {
        this->entry_menu->OnHomeButtonPress();
        return true;
    }

}