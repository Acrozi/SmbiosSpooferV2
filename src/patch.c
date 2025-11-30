/*
 * SMBIOS Patching
 */

#include "smbios.h"
#include "Config.h"
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>

// Global spoofed values
static UINT8 g_SpoofedUUID[16] = {0};
static CHAR16 g_SystemSerial[64] = {0};
static CHAR16 g_BiosSerial[64] = {0};
static CHAR16 g_BaseboardSerial[64] = {0};
static CHAR16 g_BaseboardModel[64] = {0};
static CHAR16 g_ProcessorSerial[64] = {0};

extern VOID RandomText(CHAR8* s, INTN len);

static VOID
EditRandom(
    IN SMBIOS_STRUCTURE_POINTER_CUSTOM table,
    IN SMBIOS_STRING* field
)
{
    CHAR8 buffer[258];
    RandomText(buffer, 257);

    if (field) {
        EditString(table, field, buffer);
    }
}

static VOID
EditCustomString(
    IN SMBIOS_STRUCTURE_POINTER_CUSTOM table,
    IN SMBIOS_STRING* field,
    IN CONST CHAR16* value
)
{
    if (!table.Raw || !field || !value || value[0] == 0) {
        return;
    }
    
    CHAR8 asciiValue[64];
    UINTN i;
    for (i = 0; i < 63 && value[i] != 0; i++) {
        if (value[i] < 128) {
            asciiValue[i] = (CHAR8)value[i];
        } else {
            asciiValue[i] = '?';
        }
    }
    asciiValue[i] = 0;
    
    EditString(table, field, asciiValue);
}

VOID
PatchType0(
    IN SMBIOS_STRUCTURE_TABLE* entry
)
{
    (VOID)entry;
}

VOID
PatchType1(
    IN SMBIOS_STRUCTURE_TABLE* entry
)
{
    if (entry == NULL) {
        Print(L"[FAIL] Entry is NULL\n");
        return;
    }
    
    SMBIOS_STRUCTURE_POINTER_CUSTOM table = FindTableByType(entry, SMBIOS_TYPE_SYSTEM_INFORMATION, 0);
    
    if (!table.Raw || !table.Type1) {
        Print(L"[FAIL] Type 1 (System) table not found\n");
        return;
    }
    
    Print(L"[WORK] Patching Type 1 (System) at 0x%016lx...\n", (UINT64)(UINTN)table.Raw);

    #if defined(SPOOF_SYSTEM_SERIAL) && SPOOF_SYSTEM_SERIAL
    if (g_SystemSerial[0] != 0) {
        EditCustomString(table, &table.Type1->SerialNumber, g_SystemSerial);
    } else {
        EditRandom(table, &table.Type1->SerialNumber);
    }
    #else
    EditRandom(table, &table.Type1->SerialNumber);
    #endif

    Print(L"[OK] Type 1 (System) patched successfully\n");
}

VOID
PatchType2(
    IN SMBIOS_STRUCTURE_TABLE* entry
)
{
    if (entry == NULL) {
        Print(L"[FAIL] Entry is NULL\n");
        return;
    }
    
    SMBIOS_STRUCTURE_POINTER_CUSTOM table = FindTableByType(entry, SMBIOS_TYPE_BASEBOARD_INFORMATION, 0);
    
    if (!table.Raw || !table.Type2) {
        Print(L"[FAIL] Type 2 (Baseboard) table not found\n");
        return;
    }
    
    Print(L"[WORK] Patching Type 2 (Baseboard) at 0x%016lx...\n", (UINT64)(UINTN)table.Raw);

    #if defined(SPOOF_BASEBOARD_SERIAL) && SPOOF_BASEBOARD_SERIAL
    if (g_BaseboardSerial[0] != 0) {
        EditCustomString(table, &table.Type2->SerialNumber, g_BaseboardSerial);
    } else {
        EditRandom(table, &table.Type2->SerialNumber);
    }
    #else
    EditRandom(table, &table.Type2->SerialNumber);
    #endif

    #if defined(SPOOF_BASEBOARD_MODEL) && SPOOF_BASEBOARD_MODEL
    if (g_BaseboardModel[0] != 0) {
        EditCustomString(table, &table.Type2->ProductName, g_BaseboardModel);
    } else {
        EditRandom(table, &table.Type2->ProductName);
    }
    #endif

    Print(L"[OK] Type 2 (Baseboard) patched successfully\n");
}

VOID
PatchType4(
    IN SMBIOS_STRUCTURE_TABLE* entry
)
{
    if (entry == NULL) {
        Print(L"[FAIL] Entry is NULL\n");
        return;
    }
    
    SMBIOS_STRUCTURE_POINTER_CUSTOM table = FindTableByType(entry, SMBIOS_TYPE_PROCESSOR_INFORMATION, 0);
    
    if (!table.Raw || !table.Type4) {
        Print(L"[WARN] Type 4 (Processor) table not found\n");
        return;
    }
    
    Print(L"[WORK] Patching Type 4 (Processor) at 0x%016lx...\n", (UINT64)(UINTN)table.Raw);
    
    #if defined(SPOOF_PROCESSOR_SERIAL) && SPOOF_PROCESSOR_SERIAL
    if (table.Type4->SerialNumber != 0) {
        if (g_ProcessorSerial[0] != 0) {
            EditCustomString(table, &table.Type4->SerialNumber, g_ProcessorSerial);
            Print(L"[OK] Processor Serial Number spoofed: %s\n", g_ProcessorSerial);
        } else {
            EditRandom(table, &table.Type4->SerialNumber);
            Print(L"[OK] Processor Serial Number randomized\n");
        }
    } else {
        Print(L"[WARN] Processor Serial Number field not available\n");
    }
    #endif
    
    Print(L"[OK] Type 4 (Processor) patched successfully\n");
}

VOID
PatchAll(
    IN SMBIOS_STRUCTURE_TABLE* entry
)
{
    if (entry == NULL) {
        Print(L"[FAIL] Cannot patch - entry is NULL\n");
        return;
    }
    
    Print(L"[WORK] Starting patch sequence...\n");
    PatchType1(entry);
    PatchType2(entry);
    PatchType4(entry);
    Print(L"[OK] Patch sequence completed\n");
}

VOID
GetSpoofedUUID(
    OUT UINT8* UUID
)
{
    if (UUID != NULL) {
        CopyMem(UUID, g_SpoofedUUID, 16);
    }
}

VOID
SetSpoofedUUID(
    IN UINT8* UUID
)
{
    if (UUID != NULL) {
        CopyMem(g_SpoofedUUID, UUID, 16);
    }
}

VOID
GetSpoofedSerials(
    OUT CHAR16* SystemSerial,
    OUT CHAR16* BiosSerial,
    OUT CHAR16* BaseboardSerial,
    OUT CHAR16* BaseboardModel
)
{
    if (SystemSerial != NULL) {
        CopyMem(SystemSerial, g_SystemSerial, sizeof(g_SystemSerial));
    }
    if (BiosSerial != NULL) {
        CopyMem(BiosSerial, g_BiosSerial, sizeof(g_BiosSerial));
    }
    if (BaseboardSerial != NULL) {
        CopyMem(BaseboardSerial, g_BaseboardSerial, sizeof(g_BaseboardSerial));
    }
    if (BaseboardModel != NULL) {
        CopyMem(BaseboardModel, g_BaseboardModel, sizeof(g_BaseboardModel));
    }
}

VOID
SetSpoofedSerials(
    IN CHAR16* SystemSerial,
    IN CHAR16* BiosSerial,
    IN CHAR16* BaseboardSerial,
    IN CHAR16* BaseboardModel,
    IN CHAR16* ProcessorSerial
)
{
    if (SystemSerial != NULL) {
        CopyMem(g_SystemSerial, SystemSerial, sizeof(g_SystemSerial));
    }
    if (BiosSerial != NULL) {
        CopyMem(g_BiosSerial, BiosSerial, sizeof(g_BiosSerial));
    }
    if (BaseboardSerial != NULL) {
        CopyMem(g_BaseboardSerial, BaseboardSerial, sizeof(g_BaseboardSerial));
    }
    if (BaseboardModel != NULL) {
        CopyMem(g_BaseboardModel, BaseboardModel, sizeof(g_BaseboardModel));
    }
    if (ProcessorSerial != NULL) {
        CopyMem(g_ProcessorSerial, ProcessorSerial, sizeof(g_ProcessorSerial));
    }
}

VOID
GenerateAllSpoofedValues(
    VOID
)
{
    EfiGenerateRandomUUID(g_SpoofedUUID);
    EfiGenerateRandomSerial(g_SystemSerial, 64);
    EfiGenerateRandomSerial(g_BiosSerial, 64);
    EfiGenerateRandomSerial(g_BaseboardSerial, 64);
    
    UINTN i;
    for (i = 0; i < 63 && L"Generic Baseboard"[i] != 0; i++) {
        g_BaseboardModel[i] = L"Generic Baseboard"[i];
    }
    g_BaseboardModel[i] = 0;
}

