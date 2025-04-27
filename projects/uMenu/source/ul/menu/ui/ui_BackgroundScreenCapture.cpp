#include <ul/menu/ui/ui_BackgroundScreenCapture.hpp>
#include <ul/menu/ui/ui_MenuApplication.hpp>

extern ul::menu::ui::GlobalSettings g_GlobalSettings;
extern ul::menu::ui::MenuApplication::Ref g_MenuApplication;

namespace ul::menu::ui {

    namespace {

        constexpr size_t CaptureBufferWidth = pu::ui::render::BaseScreenWidth;
        constexpr size_t CaptureBufferHeight = pu::ui::render::BaseScreenHeight;
        constexpr size_t CaptureBufferBytesPerPixel = 4;

        constexpr size_t CaptureBufferSize = CaptureBufferWidth * CaptureBufferHeight * CaptureBufferBytesPerPixel;
        static_assert(CaptureBufferSize == 0x384000);

        RawRgbaImage::Ref g_ScreenCaptureBackground;

        constexpr u8 ScreenCaptureBackgroundAlphaIncrement = 24;

        enum class SuspendedImageMode {
            ShowingAfterStart = 0,
            Focused = 1,
            HidingForResume = 2,
            NotFocused = 3,
            ShowingGainedFocus = 4,
            HidingLostFocus = 5
        };

        u8 g_ScreenCaptureBackgroundMinimumAlpha = 0xFF;
        SuspendedImageMode g_ScreenCaptureBackgroundMode = SuspendedImageMode::ShowingAfterStart;
        s32 g_ScreenCaptureBackgroundAlpha = 0xFF;

    }
    
    void InitializeScreenCaptures(const smi::MenuStartMode start_mode) {
        g_ScreenCaptureBackground = RawRgbaImage::New(0, 0);
        if(start_mode != smi::MenuStartMode::StartupMenuPostBoot) {
            auto capture_buf = new u8[CaptureBufferSize]();
            bool flag;
            if(g_GlobalSettings.IsSuspended()) {
                // Get last app capture image
                appletGetLastApplicationCaptureImageEx(capture_buf, CaptureBufferSize, &flag);
            }
            else {
                // Get last applet capture image
                appletGetLastForegroundCaptureImageEx(capture_buf, CaptureBufferSize, &flag);
            }
            appletClearCaptureBuffer(true, AppletCaptureSharedBuffer_CallerApplet, 0xFF000000);

            g_ScreenCaptureBackground->LoadImage(capture_buf, CaptureBufferWidth, CaptureBufferHeight, CaptureBufferBytesPerPixel);

            // Force-scale to 1080p
            g_ScreenCaptureBackground->SetWidth(pu::ui::render::ScreenWidth);
            g_ScreenCaptureBackground->SetHeight(pu::ui::render::ScreenHeight);
        }

        g_ScreenCaptureBackgroundMinimumAlpha = (u8)GetRequiredUiValue<u32>("suspended_app_final_alpha");
    }

    RawRgbaImage::Ref GetScreenCaptureBackground() {
        return g_ScreenCaptureBackground;
    }

    bool HasScreenCaptureBackground() {
        return g_ScreenCaptureBackground->HasImage();
    }

    void RequestResumeScreenCaptureBackground() {
        g_ScreenCaptureBackgroundMode = SuspendedImageMode::HidingForResume;
    }

    void RequestHideScreenCaptureBackground() {
        g_ScreenCaptureBackgroundMode = SuspendedImageMode::NotFocused;
        g_ScreenCaptureBackground->SetAlpha(0);
    }

    void RequestHideLoseFocusScreenCaptureBackground() {
        g_ScreenCaptureBackgroundMode = SuspendedImageMode::HidingLostFocus;
    }

    void RequestShowGainFocusScreenCaptureBackground() {
        g_ScreenCaptureBackgroundMode = SuspendedImageMode::ShowingGainedFocus;
    }

    bool IsScreenCaptureBackgroundNotTransitioning() {
        return (g_ScreenCaptureBackgroundMode == SuspendedImageMode::NotFocused) || (g_ScreenCaptureBackgroundMode == SuspendedImageMode::Focused);
    }

    bool IsScreenCaptureBackgroundFocused() {
        return g_ScreenCaptureBackgroundMode == SuspendedImageMode::Focused;
    }

    void UpdateScreenCaptureBackground(const bool after_start_stay_at_min_alpha) {
        switch(g_ScreenCaptureBackgroundMode) {
            case SuspendedImageMode::ShowingAfterStart: {
                if(after_start_stay_at_min_alpha && (g_ScreenCaptureBackgroundAlpha <= g_ScreenCaptureBackgroundMinimumAlpha)) {
                    g_ScreenCaptureBackgroundAlpha = g_ScreenCaptureBackgroundMinimumAlpha;
                    g_ScreenCaptureBackground->SetAlpha(g_ScreenCaptureBackgroundAlpha);
                    g_ScreenCaptureBackgroundMode = SuspendedImageMode::Focused;
                }
                else if(!after_start_stay_at_min_alpha && (g_ScreenCaptureBackgroundAlpha == 0)) {
                    g_ScreenCaptureBackground->SetAlpha(g_ScreenCaptureBackgroundAlpha);
                    g_ScreenCaptureBackgroundMode = SuspendedImageMode::NotFocused;
                }
                else {
                    g_ScreenCaptureBackground->SetAlpha(g_ScreenCaptureBackgroundAlpha);
                    g_ScreenCaptureBackgroundAlpha -= ScreenCaptureBackgroundAlphaIncrement;
                    if(g_ScreenCaptureBackgroundAlpha < 0) {
                        g_ScreenCaptureBackgroundAlpha = 0;
                    }
                }
                break;
            }
            case SuspendedImageMode::Focused: {
                break;
            }
            case SuspendedImageMode::HidingForResume: {
                if(g_ScreenCaptureBackgroundAlpha == 0xFF) {
                    g_ScreenCaptureBackground->SetAlpha(g_ScreenCaptureBackgroundAlpha);
                    UL_RC_ASSERT(smi::ResumeApplication());
                }
                else {
                    g_ScreenCaptureBackground->SetAlpha(g_ScreenCaptureBackgroundAlpha);
                    g_ScreenCaptureBackgroundAlpha += ScreenCaptureBackgroundAlphaIncrement;
                    if(g_ScreenCaptureBackgroundAlpha > 0xFF) {
                        g_ScreenCaptureBackgroundAlpha = 0xFF;
                    }
                }
                break;
            }
            case SuspendedImageMode::NotFocused: {
                break;
            }
            case SuspendedImageMode::ShowingGainedFocus: {
                if(g_ScreenCaptureBackgroundAlpha == g_ScreenCaptureBackgroundMinimumAlpha) {
                    g_ScreenCaptureBackground->SetAlpha(g_ScreenCaptureBackgroundAlpha);
                    g_ScreenCaptureBackgroundMode = SuspendedImageMode::Focused;
                }
                else {
                    g_ScreenCaptureBackground->SetAlpha(g_ScreenCaptureBackgroundAlpha);
                    g_ScreenCaptureBackgroundAlpha += ScreenCaptureBackgroundAlphaIncrement;
                    if(g_ScreenCaptureBackgroundAlpha > g_ScreenCaptureBackgroundMinimumAlpha) {
                        g_ScreenCaptureBackgroundAlpha = g_ScreenCaptureBackgroundMinimumAlpha;
                    }
                }
                break;
            }
            case SuspendedImageMode::HidingLostFocus: {
                if(g_ScreenCaptureBackgroundAlpha == 0) {
                    g_ScreenCaptureBackground->SetAlpha(g_ScreenCaptureBackgroundAlpha);
                    g_ScreenCaptureBackgroundMode = SuspendedImageMode::NotFocused;
                }
                else {
                    g_ScreenCaptureBackground->SetAlpha(g_ScreenCaptureBackgroundAlpha);
                    g_ScreenCaptureBackgroundAlpha -= ScreenCaptureBackgroundAlphaIncrement;
                    if(g_ScreenCaptureBackgroundAlpha < 0) {
                        g_ScreenCaptureBackgroundAlpha = 0;
                    }
                }
                break;
            }
        }
    }

}
