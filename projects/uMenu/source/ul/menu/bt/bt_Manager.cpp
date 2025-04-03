#include <ul/menu/bt/bt_Manager.hpp>
#include <ul/util/util_String.hpp>
#include <ul/util/util_Size.hpp>
#include <atomic>

using namespace ul::util::size;

namespace ul::menu::bt {

    namespace {

        Event g_AudioDeviceConnectionEvent;

        constexpr size_t MaxDiscoveredAudioDevices = 15; // qlaunch itself uses these sizes
        constexpr size_t MaxPairedAudioDevices = 10;

        constexpr BtdrvAddress InvalidAddress = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
        std::vector<BtmAudioDevice> g_PairedAudioDevices;
        RecursiveMutex g_PairedLock;
        std::atomic_bool g_PairedHasChanges = false;

        BtmAudioDevice g_ConnectedAudioDevice;
        RecursiveMutex g_ConnectedLock;
        std::atomic_bool g_ConnectedHasChanges = false;

        std::vector<BtmAudioDevice> g_DiscoveredAudioDevices;
        RecursiveMutex g_DiscoveredLock;
        std::atomic_bool g_DiscoveredHasChanges = false;

        std::atomic_bool g_ThreadRunning = true;

        inline bool IsAudioDeviceValid(const BtmAudioDevice &device) {
            return memcmp(&device.address, &InvalidAddress, sizeof(BtdrvAddress)) != 0;
        }

        inline void UpdateAudioDeviceList(std::vector<BtmAudioDevice> &dst, const std::vector<BtmAudioDevice> &src, std::atomic_bool &out_has_changes) {
            if(src.size() != dst.size()) {
                out_has_changes = true;
            }
            else if(memcmp(dst.data(), src.data(), sizeof(BtmAudioDevice) * dst.size()) != 0) {
                out_has_changes = true;
            }

            dst = src;
        }

        void ReloadConnectedDevice() {
            ScopedLock lock(g_ConnectedLock);
            
            const auto prev_connected_device_addr = g_ConnectedAudioDevice.address;
            g_ConnectedAudioDevice = {};

            // qlaunch itself only expects 1 device to be connected at a time
            s32 connected_count = 0;
            UL_RC_ASSERT(btmsysGetConnectedAudioDevices(&g_ConnectedAudioDevice, 1, &connected_count));

            g_ConnectedHasChanges = !AudioDeviceAddressesEqual(g_ConnectedAudioDevice.address, prev_connected_device_addr);
            if(g_ConnectedHasChanges) {
                if(connected_count > 0) {
                    UL_LOG_INFO("[bt] Connected device changed: connected to '%s'", g_ConnectedAudioDevice.name);
                }
                else {
                    UL_LOG_INFO("[bt] Connected device changed: no connected audio device");
                }
            }
        }
        
        void ReloadPairedDevices() {
            ScopedLock lock(g_PairedLock);

            BtmAudioDevice paired_devices[MaxPairedAudioDevices] = {};
            s32 paired_count = 0;
            std::vector<BtmAudioDevice> new_paired_devices;
            UL_RC_ASSERT(btmsysGetPairedAudioDevices(paired_devices, MaxPairedAudioDevices, &paired_count));
            for(s32 i = 0; i < paired_count; i++) {
                const auto cur_dev = paired_devices[i];
                new_paired_devices.push_back(cur_dev);
            }

            UpdateAudioDeviceList(g_PairedAudioDevices, new_paired_devices, g_PairedHasChanges);
            if(g_PairedHasChanges) {
                UL_LOG_INFO("[bt] Paired devices changed");
            }
        }

        void ReloadDiscoveredDevices() {
            ScopedLock lock(g_DiscoveredLock);

            BtmAudioDevice discovered_devices[MaxDiscoveredAudioDevices] = {};
            s32 discovered_count = 0;
            std::vector<BtmAudioDevice> new_discovered_devices;
            UL_RC_ASSERT(btmsysGetDiscoveredAudioDevice(discovered_devices, MaxDiscoveredAudioDevices, &discovered_count));
            for(s32 i = 0; i < discovered_count; i++) {
                const auto cur_dev = discovered_devices[i];
                new_discovered_devices.push_back(cur_dev);
            }

            UpdateAudioDeviceList(g_DiscoveredAudioDevices, new_discovered_devices, g_DiscoveredHasChanges);
            if(g_DiscoveredHasChanges) {
                UL_LOG_INFO("[bt] Discovered devices changed");
            }
        }

        Thread g_BluetoothThread;
        alignas(0x1000) constinit u8 g_BluetoothThreadStack[32_KB];
    
        void BluetoothThread(void*) {
            UL_LOG_INFO("[bt] Bluetooth thread alive!");
            UL_RC_ASSERT(btmsysAcquireAudioDeviceConnectionEvent(&g_AudioDeviceConnectionEvent, false));
    
            while(g_ThreadRunning) {
                const auto rc = eventWait(&g_AudioDeviceConnectionEvent, 500'000'000);
                eventClear(&g_AudioDeviceConnectionEvent);
                if(R_FAILED(rc) && (rc != KERNELRESULT(TimedOut))) {
                    UL_LOG_WARN("[bt] Failed to wait for events (and not a time-out): %s", ul::util::FormatResultDisplay(rc));
                }
                else {
                    ReloadConnectedDevice();
                    ReloadPairedDevices();
                    ReloadDiscoveredDevices();
                }
    
                svcSleepThread(1'000'000);
            }

            UL_LOG_INFO("[bt] Bluetooth thread exiting...");

            eventClose(&g_AudioDeviceConnectionEvent);
        }
    
    }

    void InitializeBluetoothManager() {
        UL_RC_ASSERT(btmsysInitialize());
        ReloadPairedDevices();
        ReloadConnectedDevice();

        UL_RC_ASSERT(threadCreate(&g_BluetoothThread, BluetoothThread, nullptr, g_BluetoothThreadStack, sizeof(g_BluetoothThreadStack), 38, -2));
        UL_RC_ASSERT(threadStart(&g_BluetoothThread));
    }

    void FinalizeBluetoothManager() {
        g_ThreadRunning = false;
        UL_RC_ASSERT(threadWaitForExit(&g_BluetoothThread));
        UL_RC_ASSERT(threadClose(&g_BluetoothThread));
        btmsysExit();
    }

    std::vector<BtmAudioDevice> ListPairedAudioDevices() {
        ScopedLock lock(g_PairedLock);
        return g_PairedAudioDevices;
    }

    bool HasPairedAudioDeviceChanges() {
        if(g_PairedHasChanges) {
            g_PairedHasChanges = false;
            return true;
        }
        return false;
    }

    BtmAudioDevice GetConnectedAudioDevice() {
        ScopedLock lock(g_ConnectedLock);
        return g_ConnectedAudioDevice;
    }

    bool HasConnectedAudioDeviceChanges() {
        if(g_ConnectedHasChanges) {
            g_ConnectedHasChanges = false;
            return true;
        }
        return false;
    }

    std::vector<BtmAudioDevice> ListDiscoveredAudioDevices() {
        ScopedLock lock(g_DiscoveredLock);
        return g_DiscoveredAudioDevices;
    }

    bool HasDiscoveredAudioDeviceChanges() {
        if(g_DiscoveredHasChanges) {
            g_DiscoveredHasChanges = false;
            return true;
        }
        return false;
    }

    Result ConnectAudioDevice(const BtmAudioDevice &device) {
        g_ConnectedHasChanges = true;
        g_DiscoveredHasChanges = true;
        g_PairedHasChanges = true;
        return btmsysConnectAudioDevice(device.address);
    }

    Result DisconnectAudioDevice(const BtmAudioDevice &device) {
        g_ConnectedHasChanges = true;
        return btmsysDisconnectAudioDevice(device.address);
    }

    Result UnpairAudioDevice(const BtmAudioDevice &device) {
        g_PairedHasChanges = true;
        g_DiscoveredHasChanges = true;
        return btmsysRemoveAudioDevicePairing(device.address);
    }

    void StartAudioDeviceDiscovery() {
        const auto rc = btmsysStartAudioDeviceDiscovery();
        if(R_FAILED(rc)) {
            UL_LOG_WARN("[bt] Failed to start audio device discovery: %s", ul::util::FormatResultDisplay(rc).c_str());
        }
        else {
            UL_LOG_INFO("[bt] Starting bluetooth audio device discovery...");
        }
    }

    void StopAudioDeviceDiscovery() {
        const auto rc = btmsysStopAudioDeviceDiscovery();
        if(R_FAILED(rc)) {
            UL_LOG_WARN("[bt] Failed to stop audio device discovery: %s", ul::util::FormatResultDisplay(rc).c_str());
        }
        else {
            UL_LOG_INFO("[bt] Stopping bluetooth audio device discovery...");
        }
    }

}
