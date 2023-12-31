#include <switch.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char g_noticeText[] =
    "uHbTarget as part of uLaunch v" UL_VERSION " :)" "\0"
    "There is only one true CFW in the scene.";

static char g_argv[2048];
static char g_nextArgv[2048];
static char g_nextNroPath[512];
u64  g_nroAddr = 0;
static u64  g_nroSize = 0;
static NroHeader g_nroHeader;

static bool g_isApplication = 0;

static NsApplicationControlData g_applicationControlData;
static bool g_isAutomaticGameplayRecording = 0;
static bool g_smCloseWorkaround = false;

static u64 g_appletHeapSize = 0;
static u64 g_appletHeapReservationSize = 0;

static u128 g_userIdStorage;

static u8 g_savedTls[0x100];

// Special hbtarget stuff
static char g_basePath[2048];
static char g_baseArgv[2048];
static int g_targetCounter = 1;

// Used by trampoline.s
Result g_lastRet = 0;

extern void* __stack_top;//Defined in libnx.
extern u32 __nx_applet_type;
#define STACK_SIZE 0x100000 //Change this if main-thread stack size ever changes.

void __libnx_initheap(void)
{
    static char g_innerheap[0x20000];

    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = &g_innerheap[0];
    fake_heap_end   = &g_innerheap[sizeof g_innerheap];
}

static Result readSetting(const char* key, void* buf, size_t size)
{
    Result rc;
    u64 actual_size;
    const char* const section_name = "hbloader";
    rc = setsysGetSettingsItemValueSize(section_name, key, &actual_size);
    if (R_SUCCEEDED(rc) && actual_size != size)
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (R_SUCCEEDED(rc))
        rc = setsysGetSettingsItemValue(section_name, key, buf, size, &actual_size);
    if (R_SUCCEEDED(rc) && actual_size != size)
        rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    if (R_FAILED(rc)) memset(buf, 0, size);
    return rc;
}

void __appInit(void)
{
    // Init sm, init set:sys, close sm
    Result rc;

    rc = smInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 1));

    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        readSetting("applet_heap_size", &g_appletHeapSize, sizeof(g_appletHeapSize));
        readSetting("applet_heap_reservation_size", &g_appletHeapReservationSize, sizeof(g_appletHeapReservationSize));
        setsysExit();
    }

    smExit();
}

void __appExit(void)
{
}

static void*  g_heapAddr;
static size_t g_heapSize;

static u64 calculateMaxHeapSize(void)
{
    u64 size = 0;
    u64 mem_available = 0, mem_used = 0;

    svcGetInfo(&mem_available, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&mem_used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);

    if (mem_available > mem_used+0x200000)
        size = (mem_available - mem_used - 0x200000) & ~0x1FFFFF;
    if (size == 0)
        size = 0x2000000*16;
    if (size > 0x6000000 && g_isAutomaticGameplayRecording)
        size -= 0x6000000;

    return size;
}

static void setupHbHeap(void)
{
    void* addr = NULL;
    u64 size = calculateMaxHeapSize();

    if (!g_isApplication && (__nx_applet_type != AppletType_None)) {
        if (g_appletHeapSize) {
            u64 requested_size = (g_appletHeapSize + 0x1FFFFF) &~ 0x1FFFFF;
            if (requested_size < size)
                size = requested_size;
        }
        else if (g_appletHeapReservationSize) {
            u64 reserved_size = (g_appletHeapReservationSize + 0x1FFFFF) &~ 0x1FFFFF;
            if (reserved_size < size)
                size -= reserved_size;
        }
    }

    Result rc = svcSetHeapSize(&addr, size);

    if (R_FAILED(rc) || addr==NULL)
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 9));

    g_heapAddr = addr;
    g_heapSize = size;
}

static Handle g_procHandle;

//Gets the control.nacp for the current title id, and then sets g_isAutomaticGameplayRecording if less memory should be allocated.
static void getIsAutomaticGameplayRecording(void) {
    if (hosversionAtLeast(5,0,0) && g_isApplication) {
        Result rc=0;
        u64 cur_tid=0;

        rc = svcGetInfo(&cur_tid, InfoType_ProgramId, CUR_PROCESS_HANDLE, 0);
        if (R_FAILED(rc)) return;

        g_isAutomaticGameplayRecording = 0;

        rc = smInitialize();
        if(R_SUCCEEDED(rc)) {
            rc = nsInitialize();

            if (R_SUCCEEDED(rc)) {
                size_t dummy;
                rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, cur_tid, &g_applicationControlData, sizeof(g_applicationControlData), &dummy);
                nsExit();
            }

            if (R_SUCCEEDED(rc) && g_applicationControlData.nacp.video_capture == 2) g_isAutomaticGameplayRecording = 1;
            smExit();
        }
    }
}

Result sdInitMount()
{
    // When loading NRO: init sm, init fs, mount sd, unmount sd, close fs, close sm
    Result rc = smInitialize();
    if(R_SUCCEEDED(rc))
    {
        rc = fsInitialize();
        if(R_SUCCEEDED(rc)) rc = fsdevMountSdmc();
    }
    return rc;
}

void sdCloseUnmount()
{
    fsdevUnmountAll();
    fsExit();
    smExit();
}

void loadNro(void)
{
    if(g_targetCounter >= 0)
    {
        if(g_targetCounter == 0) exit(0);
        g_targetCounter--;
    }

    NroHeader* header = NULL;
    size_t rw_size=0;

    Result rc = sdInitMount();
    if(R_FAILED(rc)) fatalThrow(rc);

    if (g_smCloseWorkaround) {
        // For old applications, wait for SM to handle closing the SM session from this process.
        // If we don't do this, smInitialize will fail once eventually used later.
        // This is caused by a bug in old versions of libnx that was fixed in commit 68a77ac950.
        g_smCloseWorkaround = false;
        svcSleepThread(1000000000);
    }

    memcpy((u8*)armGetTls() + 0x100, g_savedTls, 0x100);

    if (g_nroSize > 0)
    {
        // Unmap previous NRO.
        header = &g_nroHeader;
        rw_size = header->segments[2].size + header->bss_size;
        rw_size = (rw_size+0xFFF) & ~0xFFF;

        // .text
        rc = svcUnmapProcessCodeMemory(
            g_procHandle, g_nroAddr + header->segments[0].file_off, ((u64) g_heapAddr) + header->segments[0].file_off, header->segments[0].size);

        if (R_FAILED(rc))
            fatalThrow(MAKERESULT(Module_HomebrewLoader, 24));

        // .rodata
        rc = svcUnmapProcessCodeMemory(
            g_procHandle, g_nroAddr + header->segments[1].file_off, ((u64) g_heapAddr) + header->segments[1].file_off, header->segments[1].size);

        if (R_FAILED(rc))
            fatalThrow(MAKERESULT(Module_HomebrewLoader, 25));

       // .data + .bss
        rc = svcUnmapProcessCodeMemory(
            g_procHandle, g_nroAddr + header->segments[2].file_off, ((u64) g_heapAddr) + header->segments[2].file_off, rw_size);

        if (R_FAILED(rc))
            fatalThrow(MAKERESULT(Module_HomebrewLoader, 26));

        g_nroAddr = g_nroSize = 0;
    }

    if (strlen(g_nextNroPath) == 0)
    {
        strcpy(g_nextNroPath, g_basePath);
        strcpy(g_nextArgv, g_baseArgv);
    }

    memcpy(g_argv, g_nextArgv, sizeof g_argv);

    uint8_t *nrobuf = (uint8_t*) g_heapAddr;

    NroStart*  start  = (NroStart*)  (nrobuf + 0);
    header = (NroHeader*) (nrobuf + sizeof(NroStart));
    uint8_t*   rest   = (uint8_t*)   (nrobuf + sizeof(NroStart) + sizeof(NroHeader));

    FILE* f = fopen(g_nextNroPath, "rb");
    if (f == NULL)
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 3));

    // Reset NRO path to load hbmenu by default next time.
    g_nextNroPath[0] = '\0';

    if (fread(start, sizeof(*start), 1, f) != 1)
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 4));

    if (fread(header, sizeof(*header), 1, f) != 1)
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 4));

    if(header->magic != NROHEADER_MAGIC)
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 5));

    size_t rest_size = header->size - (sizeof(NroStart) + sizeof(NroHeader));
    if (fread(rest, rest_size, 1, f) != 1)
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 7));

    fclose(f);

    sdCloseUnmount();

    size_t total_size = header->size + header->bss_size;
    total_size = (total_size+0xFFF) & ~0xFFF;

    rw_size = header->segments[2].size + header->bss_size;
    rw_size = (rw_size+0xFFF) & ~0xFFF;

    bool has_mod0 = false;
    if (start->mod_offset > 0 && start->mod_offset <= (total_size-0x24)) // Validate MOD0 offset
        has_mod0 = *(uint32_t*)(nrobuf + start->mod_offset) == 0x30444F4D; // Validate MOD0 header

    int i;
    for (i=0; i<3; i++)
    {
        if (header->segments[i].file_off >= header->size || header->segments[i].size > header->size ||
            (header->segments[i].file_off + header->segments[i].size) > header->size)
        {
            fatalThrow(MAKERESULT(Module_HomebrewLoader, 6));
        }
    }

    // todo: Detect whether NRO fits into heap or not.

    // Copy header to elsewhere because we're going to unmap it next.
    memcpy(&g_nroHeader, header, sizeof(g_nroHeader));
    header = &g_nroHeader;

    u64 map_addr;

    do {
        map_addr = randomGet64() & 0xFFFFFF000ull;
        rc = svcMapProcessCodeMemory(g_procHandle, map_addr, (u64)nrobuf, total_size);

    } while (rc == 0xDC01 || rc == 0xD401);

    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 18));

    // .text
    rc = svcSetProcessMemoryPermission(
        g_procHandle, map_addr + header->segments[0].file_off, header->segments[0].size, Perm_R | Perm_X);

    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 19));

    // .rodata
    rc = svcSetProcessMemoryPermission(
        g_procHandle, map_addr + header->segments[1].file_off, header->segments[1].size, Perm_R);

    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 20));

    // .data + .bss
    rc = svcSetProcessMemoryPermission(
        g_procHandle, map_addr + header->segments[2].file_off, rw_size, Perm_Rw);

    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_HomebrewLoader, 21));

    u64 nro_size = header->segments[2].file_off + rw_size;
    u64 nro_heap_start = ((u64) g_heapAddr) + nro_size;
    u64 nro_heap_size  = g_heapSize + (u64) g_heapAddr - (u64) nro_heap_start;

    #define M EntryFlag_IsMandatory

    static ConfigEntry entries[] = {
        { EntryType_MainThreadHandle,     0, {0, 0} },
        { EntryType_ProcessHandle,        0, {0, 0} },
        { EntryType_AppletType,           0, {AppletType_None, 0} },
        { EntryType_OverrideHeap,         M, {0, 0} },
        { EntryType_Argv,                 0, {0, 0} },
        { EntryType_NextLoadPath,         0, {0, 0} },
        { EntryType_LastLoadResult,       0, {0, 0} },
        { EntryType_SyscallAvailableHint, 0, {0xffffffffffffffff, 0x9fc1fff0007ffff} },
        { EntryType_RandomSeed,           0, {0, 0} },
        { EntryType_UserIdStorage,        0, {(u64)(uintptr_t)&g_userIdStorage, 0} },
        { EntryType_HosVersion,           0, {0, 0} },
        { EntryType_EndOfList,            0, {(u64)(uintptr_t)g_noticeText, sizeof(g_noticeText)} }
    };

    ConfigEntry *entry_AppletType = &entries[2];
    entry_AppletType->Value[0] = __nx_applet_type;

    if (g_isApplication) {
        entry_AppletType->Value[1] = EnvAppletFlags_ApplicationOverride;
    }

    // MainThreadHandle
    entries[0].Value[0] = envGetMainThreadHandle();
    // ProcessHandle
    entries[1].Value[0] = g_procHandle;
    // OverrideHeap
    entries[3].Value[0] = nro_heap_start;
    entries[3].Value[1] = nro_heap_size;
    // Argv
    entries[4].Value[1] = (u64) &g_argv[0];
    // NextLoadPath
    entries[5].Value[0] = (u64) &g_nextNroPath[0];
    entries[5].Value[1] = (u64) &g_nextArgv[0];
    // LastLoadResult
    entries[6].Value[0] = g_lastRet;
    // RandomSeed
    entries[8].Value[0] = randomGet64();
    entries[8].Value[1] = randomGet64();
    // HosVersion
    entries[10].Value[0] = hosversionGet();

    u64 entrypoint = map_addr;

    g_nroAddr = map_addr;
    g_nroSize = nro_size;

    memset(__stack_top - STACK_SIZE, 0, STACK_SIZE);

    if (!has_mod0) {
        // Apply sm-close workaround to NROs which do not contain a valid MOD0 header.
        // This heuristic is based on the fact that MOD0 support was added very shortly after
        // the fix for the sm-close bug (in fact, two commits later).
        g_smCloseWorkaround = true;
    }

    extern NX_NORETURN void hbTargetImpl(u64 entries_ptr, u64 handle, u64 entrypoint);
    hbTargetImpl((u64) entries, -1, entrypoint);
}

void hb_hbl_Target(Handle process_handle, const char *path, const char *argv, bool do_once) {
    g_procHandle = process_handle;
    g_targetCounter = -1;
    if(do_once) {
        g_targetCounter = 1;
    }
    g_isApplication = (__nx_applet_type == AppletType_Application) || (__nx_applet_type == AppletType_SystemApplication);
    memcpy(g_savedTls, (u8*)armGetTls() + 0x100, 0x100);
    strcpy(g_basePath, path);
    strcpy(g_baseArgv, argv);

    getIsAutomaticGameplayRecording();
    setupHbHeap();
    loadNro();
}