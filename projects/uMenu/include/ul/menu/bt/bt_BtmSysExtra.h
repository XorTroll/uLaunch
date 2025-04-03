
#ifndef UL_BTM_SYS_EXTRA_H
#define UL_BTM_SYS_EXTRA_H

#ifdef __cplusplus
extern "C" {
#endif

// Thanks to ndeadly for the definitions

#include <switch.h>

typedef struct {
    BtdrvAddress address;
    char name[0xF9];
} BtmAudioDevice;

Result btmsysStartAudioDeviceDiscovery(void);

Result btmsysStopAudioDeviceDiscovery(void);

Result btmsysIsDiscoveryingAudioDevice(bool *out);

// qlaunch uses count=15
Result btmsysGetDiscoveredAudioDevice(BtmAudioDevice *devices, s32 count, s32 *total_out);

Result btmsysAcquireAudioDeviceConnectionEvent(Event *out_event, bool autoclear);

Result btmsysConnectAudioDevice(BtdrvAddress address);

Result btmsysIsConnectingAudioDevice(bool *out);

// qlaunch uses count=1 (there can only be a single connected device)
Result btmsysGetConnectedAudioDevices(BtmAudioDevice *devices, s32 count, s32 *total_out);

Result btmsysDisconnectAudioDevice(BtdrvAddress address);

Result btmsysAcquirePairedAudioDeviceInfoChangedEvent(Event *out_event, bool autoclear);

// qlaunch uses count=10
Result btmsysGetPairedAudioDevices(BtmAudioDevice *devices, s32 count, s32 *total_out);

Result btmsysRemoveAudioDevicePairing(BtdrvAddress address);

Result btmsysRequestAudioDeviceConnectionRejection(u64 aruid);

Result btmsysCancelAudioDeviceConnectionRejection(u64 aruid);

#ifdef __cplusplus
}
#endif

#endif
