
#pragma once
#include <vector>
#include <ul/ul_Include.hpp>

namespace ul::os {

    constexpr u64 InvalidApplicationId = 0;

    // All view flags I found to be used by qlaunch (names are pure guesses)

    enum class ApplicationViewFlag : u32 {
        IsValid = BIT(0), // So far I have never seen this flag unset
        HasMainContents = BIT(1), // Games might have updates/DLC but not the main content
        HasContentsInstalled = BIT(4), // This flag is set for all installed applications with contents
        IsDownloading = BIT(5), // qlaunch calls GetApplicationViewDownloadErrorContext somewhere if this flag is set
        IsGameCard = BIT(6), // qlaunch calls EnsureGameCardAccess somewhere if this flag is set
        IsGameCardInserted = BIT(7), // qlaunch checks this flag along with the one above in many places, but this one is only set on gamecard games that are inserted
        CanLaunch = BIT(8), // qlaunch calls CheckApplicationLaunchVersion somewhere if this flag is set (and pctl ConfirmLaunchApplicationPermission somewhere else)
        NeedsUpdate = BIT(9), // qlaunch calls RequestApplicationUpdate somewhere if this flag is set, if IsValid is also set and if IsDownloading is not set
        CanLaunch2 = BIT(10), // Games seem to have set it along with CanLaunch
        NeedsVerify = BIT(13), // Games which need to be verified by qlaunch (due to corrupted data/etc) have this flag set
        IsWaitingCommit1 = BIT(14), // I guess, from "OptCntUpdate_DlgBodyWaitCommit" texts used if these flags are all set
        IsWaitingCommit2 = BIT(15),
        IsWaitingCommit3 = BIT(18),
        
        Bit23 = BIT(23),
        Bit16 = BIT(16),
        IsApplyingDelta = BIT(17), // I guess, from "IsApplyingDelta" text used if this flag is set
        Bit21 = BIT(21),
        Bit22 = BIT(22), // PromotionInfo related?
    };

    struct ApplicationView {
        u64 app_id;
        u32 used32_x8;
        u32 flags;
        u64 download_progress_current;
        u64 download_progress_total;
        u32 used32_x20;
        u8 used8_x24_download_rel;
        u8 download_has_eta;
        u8 used8_x26;
        u8 unk_x27;
        u64 used64_x28;
        u64 other_progress_current;
        u64 other_progress_total;
        u32 used32_x40;
        u8 used8_x44;
        u8 used8_x45;
        u8 unk_x46[2];
        u64 used64_x48;

        template<ApplicationViewFlag Flag>
        inline constexpr bool HasFlag() const {
            return (this->flags & static_cast<u32>(Flag)) != 0;
        }
    };
    static_assert(sizeof(ApplicationView) == sizeof(NsApplicationView));

    std::vector<NsApplicationRecord> ListApplicationRecords();
    std::vector<os::ApplicationView> ListApplicationViews(const std::vector<NsApplicationRecord> &base_records);

}
