
#pragma once
#include <ul/ul_Result.hpp>
#include <ul/menu/bt/bt_BtmSysExtra.h>
#include <cstring>

namespace ul::menu::bt {

    void InitializeBluetoothManager();
    void FinalizeBluetoothManager();

    inline bool AudioDeviceAddressesEqual(const BtdrvAddress &a, const BtdrvAddress &b) {
        return memcmp(std::addressof(a), std::addressof(b), sizeof(BtdrvAddress)) == 0;
    }
    
    inline bool AudioDevicesEqual(const BtmAudioDevice &a, const BtmAudioDevice &b) {
        return AudioDeviceAddressesEqual(a.address, b.address);
    }

    std::vector<BtmAudioDevice> ListPairedAudioDevices();
    bool HasPairedAudioDeviceChanges();

    BtmAudioDevice GetConnectedAudioDevice();
    bool HasConnectedAudioDeviceChanges();

    std::vector<BtmAudioDevice> ListDiscoveredAudioDevices();
    bool HasDiscoveredAudioDeviceChanges();

    Result ConnectAudioDevice(const BtmAudioDevice &device);
    Result DisconnectAudioDevice(const BtmAudioDevice &device);
    Result UnpairAudioDevice(const BtmAudioDevice &device);

    void StartAudioDeviceDiscovery();
    void StopAudioDeviceDiscovery();

}
