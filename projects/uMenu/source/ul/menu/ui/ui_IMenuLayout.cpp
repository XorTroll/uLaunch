#include <ul/menu/ui/ui_IMenuLayout.hpp>
#include <ul/menu/ui/ui_Common.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>
#include <ul/net/net_Service.hpp>

extern ul::menu::ui::GlobalSettings g_GlobalSettings;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    namespace {

        s32 g_HomeButtonPressHandleCount = 0;

        std::vector<std::string> g_WeekdayList;

        void EnsureWeekdayList() {
            if(g_WeekdayList.empty()) {
                for(u32 i = 0; i < 7; i++) {
                    g_WeekdayList.push_back(GetLanguageString("week_day_short_" + std::to_string(i)));
                }
            }
        }

        void OnFinishedSleep() {
            // Reset and reinitialize audio (force-avoid post-sleep audio stutter in audout)
            g_MenuApplication->DisposeAllSfx();
            ul::menu::ui::DisposeAllBgm();
            pu::audio::Finalize();
    
            UL_ASSERT_TRUE(pu::audio::Initialize(MIX_INIT_MP3));
            g_MenuApplication->LoadBgmSfxForCreatedMenus();
    
            // Load lockscreen, if enabled
            bool lockscreen_enabled;
            UL_ASSERT_TRUE(g_GlobalSettings.config.GetEntry(cfg::ConfigEntryId::LockscreenEnabled, lockscreen_enabled));
            if(lockscreen_enabled) {
                g_MenuApplication->LoadMenu(MenuType::Lockscreen, true);
            }
    
            g_MenuApplication->StartPlayBgm();
        }

    }

    void IMenuLayout::UpdateConnectionTopIcon(pu::ui::elm::Image::Ref &icon) {
        u32 conn_strength;
        const auto has_conn = net::HasConnection(conn_strength);
        if((this->last_has_connection != has_conn) || (this->last_connection_strength != conn_strength)) {
            this->last_has_connection = has_conn;
            this->last_connection_strength = conn_strength;
            if(has_conn) {
                icon->SetImage(TryFindLoadImageHandle("ui/Main/TopIcon/Connection/" + std::to_string(conn_strength)));
            }
            else {
                icon->SetImage(TryFindLoadImageHandle("ui/Main/TopIcon/Connection/None"));
            }
        }
    }

    void IMenuLayout::UpdateDateText(pu::ui::elm::TextBlock::Ref &date_text) {
        const auto cur_date = os::GetCurrentDate();
        if(this->last_date != cur_date) {
            this->last_date = cur_date;

            char cur_date_str[0x40] = {};
            sprintf(cur_date_str, "%02d/%02d (%s)", cur_date.day, cur_date.month, g_WeekdayList.at(cur_date.weekday_idx).c_str());
            date_text->SetText(cur_date_str);
        }
    }

    void IMenuLayout::InitializeTimeText(MultiTextBlock::Ref &time_mtext, const std::string &ui_menu, const std::string &ui_name) {
        time_mtext = MultiTextBlock::New(0, 0);
        time_mtext->Add(pu::ui::elm::TextBlock::New(0, 0, "99"));
        time_mtext->Add(pu::ui::elm::TextBlock::New(0, 0, ":"));
        time_mtext->Add(pu::ui::elm::TextBlock::New(0, 0, "99"));
        g_GlobalSettings.ApplyConfigForElement(ui_menu, ui_name, time_mtext);
        for(auto &text: time_mtext->GetAll()) {
            text->SetColor(g_MenuApplication->GetTextColor());
        }
        time_mtext->UpdatePositionsSizes();
        this->Add(time_mtext);
        this->Add(time_mtext->Get(0));
        this->Add(time_mtext->Get(1));
        this->Add(time_mtext->Get(2));
    }

    void IMenuLayout::UpdateTimeText(MultiTextBlock::Ref &time_mtext) {
        const auto cur_time = os::GetCurrentTime();
        auto time_changed = false;

        if(this->last_time.h != cur_time.h) {
            time_changed = true;
            char cur_h_str[0x40] = {};
            sprintf(cur_h_str, "%02d", cur_time.h);
            time_mtext->Get(0)->SetText(cur_h_str);
        }

        if(this->last_time.min != cur_time.min) {
            time_changed = true;
            char cur_min_str[0x40] = {};
            sprintf(cur_min_str, "%02d", cur_time.min);
            time_mtext->Get(2)->SetText(cur_min_str);
        }

        this->time_anim_frame++;
        if(this->time_anim_frame > TimeDotsAnimStepCount) {
            this->time_anim_frame = 0;
            this->time_anim_dots = !this->time_anim_dots;
            time_mtext->Get(1)->SetVisible(this->time_anim_dots);
        }

        if(time_changed) {
            this->last_time = cur_time;
        }
    }

    void IMenuLayout::UpdateBatteryTextAndTopIcons(pu::ui::elm::TextBlock::Ref &text, pu::ui::elm::Image::Ref &base_top_icon, pu::ui::elm::Image::Ref &charging_top_icon) {
        const auto battery_level = os::GetBatteryLevel();
        const auto is_charging = os::IsConsoleCharging();
        if((this->last_battery_level != battery_level) || (this->last_battery_is_charging != is_charging)) {
            this->last_battery_level = battery_level;
            this->last_battery_is_charging = is_charging;

            const auto battery_str = std::to_string(battery_level) + "%";
            text->SetText(battery_str);

            auto battery_lvl_norm = (1 + (battery_level / 10)) * 10; // Converts it to 10, 20, ..., 100
            if(battery_lvl_norm > 100) {
                battery_lvl_norm = 100;
            }
            const auto battery_img = "ui/Main/TopIcon/Battery/" + std::to_string(battery_lvl_norm);
            base_top_icon->SetImage(TryFindLoadImageHandle(battery_img));
            charging_top_icon->SetVisible(is_charging);
        }
    }

    IMenuLayout::IMenuLayout() : Layout(), msg_queue_lock(), msg_queue(), last_has_connection(false), last_connection_strength(0), last_battery_level(0), last_battery_is_charging(false), last_time(), last_date(), time_anim_frame(0), time_anim_dots(true) {
        this->SetBackgroundImage(GetBackgroundTexture());
        this->SetOnInput(std::bind(&IMenuLayout::OnLayoutInput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        this->AddRenderCallback(std::bind(&IMenuLayout::OnMenuUpdate, this));

        EnsureWeekdayList();
    }

    void IMenuLayout::OnLayoutInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const pu::ui::TouchPoint touch_pos) {
        {
            ScopedLock lk(this->msg_queue_lock);

            if(!this->msg_queue.empty()) {
                const auto first_msg = this->msg_queue.front();

                switch(first_msg.msg) {
                    case smi::MenuMessage::HomeRequest: {
                        g_HomeButtonPressHandleCount++;
                        if(g_HomeButtonPressHandleCount == 1) {
                            if(this->OnHomeButtonPress()) {
                                g_HomeButtonPressHandleCount = 0;
                                this->msg_queue.pop();
                            }
                        }
                        break;
                    }
                    case smi::MenuMessage::GameCardMountFailure: {
                        g_MenuApplication->NotifyGameCardMountFailure(first_msg.gc_mount_failure.mount_rc);
                        this->msg_queue.pop();
                        break;
                    }
                    case smi::MenuMessage::SdCardEjected: {
                        this->msg_queue.pop();
                        while(true) {
                            const auto option = g_MenuApplication->DisplayDialog(GetLanguageString("sd_card"), GetLanguageString("sd_card_ejected"), { GetLanguageString("shutdown"), GetLanguageString("reboot") }, false);
                            if(option == 0) {
                                ShutdownSystem();
                            }
                            else if(option == 1) {
                                RebootSystem();
                            }
                        }
                        break;
                    }
                    case smi::MenuMessage::PreviousLaunchFailure: {
                        g_MenuApplication->NotifyLaunchFailed();
                        this->msg_queue.pop();
                        break;
                    }
                    case smi::MenuMessage::ChosenHomebrew: {
                        g_MenuApplication->NotifyHomebrewChosen(first_msg.chosen_hb.nro_path);
                        this->msg_queue.pop();
                        break;
                    }
                    case smi::MenuMessage::FinishedSleep: {
                        this->msg_queue.pop();
                        OnFinishedSleep();
                        break;
                    }
                    case smi::MenuMessage::ApplicationRecordsChanged: {
                        g_MenuApplication->NotifyApplicationRecordReloadNeeded();
                        if(first_msg.app_records_changed.records_added_or_deleted) {
                            // Need to also reload entries as well
                            g_MenuApplication->NotifyApplicationEntryReloadNeeded();
                        }
                        this->msg_queue.pop();
                        break;
                    }
                    case smi::MenuMessage::ApplicationVerifyProgress: {
                        const auto progress = (float)first_msg.app_verify_progress.done / (float)first_msg.app_verify_progress.total;
                        g_MenuApplication->NotifyApplicationVerifyProgress(first_msg.app_verify_progress.app_id, progress);

                        this->msg_queue.pop();
                        break;
                    }
                    case smi::MenuMessage::ApplicationVerifyResult: {
                        g_MenuApplication->NotifyApplicationVerifyProgress(first_msg.app_verify_rc.app_id, NAN);
                        g_MenuApplication->NotifyVerifyFinished(first_msg.app_verify_rc.app_id, first_msg.app_verify_rc.rc, first_msg.app_verify_rc.detail_rc);
                        
                        this->msg_queue.pop();
                        break;
                    }
                    default: {
                        this->msg_queue.pop();
                        break;
                    }
                }
            }
        }

        this->OnMenuInput(keys_down, keys_up, keys_held, touch_pos);
    }

    void IMenuLayout::NotifyMessageContext(const smi::MenuMessageContext &msg_ctx) {
        ScopedLock lk(this->msg_queue_lock);

        // Remove consequent homemenu requests
        if(msg_ctx.msg == smi::MenuMessage::HomeRequest) {
            if(!this->msg_queue.empty()) {
                if(this->msg_queue.front().msg == smi::MenuMessage::HomeRequest) {
                    return;
                }
            }
        }
        else if(msg_ctx.msg == smi::MenuMessage::ApplicationVerifyProgress) {
            if(!this->msg_queue.empty()) {
                if(this->msg_queue.front().msg == smi::MenuMessage::ApplicationVerifyProgress) {
                    this->msg_queue.pop();
                }
            }
        }

        this->msg_queue.push(msg_ctx);
    }

}
