#include <ul/menu/bt/bt_BtmSysExtra.h>

Result btmsysStartAudioDeviceDiscovery(void) {
    return serviceDispatch(btmsysGetServiceSession_IBtmSystemCore(), 10);
}

Result btmsysStopAudioDeviceDiscovery(void) {
    return serviceDispatch(btmsysGetServiceSession_IBtmSystemCore(), 11);
}

Result btmsysIsDiscoveryingAudioDevice(bool *out) {
    return serviceDispatchOut(btmsysGetServiceSession_IBtmSystemCore(), 12, *out);
}

Result btmsysGetDiscoveredAudioDevice(BtmAudioDevice *devices, s32 count, s32 *total_out) {
    return serviceDispatchOut(btmsysGetServiceSession_IBtmSystemCore(), 13, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { devices, sizeof(BtmAudioDevice) * count } },
    );
}

Result btmsysAcquireAudioDeviceConnectionEvent(Event *out_event, bool autoclear) {
    Handle tmp_handle = INVALID_HANDLE;

    Result rc = serviceDispatch(btmsysGetServiceSession_IBtmSystemCore(), 14,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );

    if(R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

Result btmsysConnectAudioDevice(BtdrvAddress address) {
    return serviceDispatchIn(btmsysGetServiceSession_IBtmSystemCore(), 15, address);
}

Result btmsysIsConnectingAudioDevice(bool *out) {
    return serviceDispatchOut(btmsysGetServiceSession_IBtmSystemCore(), 16, *out);
}

Result btmsysGetConnectedAudioDevices(BtmAudioDevice *devices, s32 count, s32 *total_out) {
    return serviceDispatchOut(btmsysGetServiceSession_IBtmSystemCore(), 17, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { devices, sizeof(BtmAudioDevice) * count } },
    );
}

Result btmsysDisconnectAudioDevice(BtdrvAddress address) {
    return serviceDispatchIn(btmsysGetServiceSession_IBtmSystemCore(), 18, address);
}

Result btmsysAcquirePairedAudioDeviceInfoChangedEvent(Event *out_event, bool autoclear) {
    Handle tmp_handle = INVALID_HANDLE;

    Result rc = serviceDispatch(btmsysGetServiceSession_IBtmSystemCore(), 19,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &tmp_handle,
    );

    if(R_SUCCEEDED(rc)) eventLoadRemote(out_event, tmp_handle, autoclear);
    return rc;
}

Result btmsysGetPairedAudioDevices(BtmAudioDevice *devices, s32 count, s32 *total_out) {
    return serviceDispatchOut(btmsysGetServiceSession_IBtmSystemCore(), 20, *total_out,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers = { { devices, sizeof(BtmAudioDevice) * count } },
    );
}

Result btmsysRemoveAudioDevicePairing(BtdrvAddress address) {
    return serviceDispatchIn(btmsysGetServiceSession_IBtmSystemCore(), 21, address);
}

Result btmsysRequestAudioDeviceConnectionRejection(u64 aruid) {
    return serviceDispatchIn(btmsysGetServiceSession_IBtmSystemCore(), 22, aruid,
        .in_send_pid = true,
    );
}

Result btmsysCancelAudioDeviceConnectionRejection(u64 aruid) {
    return serviceDispatchIn(btmsysGetServiceSession_IBtmSystemCore(), 23, aruid,
        .in_send_pid = true,
    );
}
