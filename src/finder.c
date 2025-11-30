/*
 * SMBIOS Entry Point Finder V2
 * EXACT COPY of negativespoofer's finder.c converted to EDK2
 */

#include "smbios.h"
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Uefi/UefiBaseType.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

// SMBIOS Table GUIDs
EFI_GUID gSmbiosTableGuid = { 0xEB9D2D31, 0x2D88, 0x11D3, { 0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D } };
EFI_GUID gSmbios3TableGuid = { 0xF2FD1544, 0x9794, 0x4A2C, { 0x99, 0x2E, 0xE5, 0xBB, 0xCF, 0x20, 0xE3, 0x94 } };
EFI_GUID gHobGuid = { 0x7739f24c, 0x93d7, 0x11d4, { 0x9a, 0x3a, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } };

#define GET_GUID_HOB_DATA(GuidHob) ((VOID*)(((UINT8*)&((GuidHob)->Name)) + sizeof(EFI_GUID)))

#define GET_HOB_TYPE(Hob) ((Hob).Header->HobType)
#define GET_HOB_LENGTH(Hob) ((Hob).Header->HobLength)
#define GET_NEXT_HOB(Hob) ((Hob).Raw + GET_HOB_LENGTH(Hob))
#define END_OF_HOB_LIST(Hob) (GET_HOB_TYPE(Hob) == EFI_HOB_TYPE_END_OF_HOB_LIST)

static VOID* gHobList = NULL;

/**
 * Get HOB List
 */
static VOID*
GetHobList(
    VOID
)
{
    if (!gHobList) {
        if (gST != NULL && gST->ConfigurationTable != NULL) {
            UINTN i;
            for (i = 0; i < gST->NumberOfTableEntries; i++) {
                if (CompareGuid(&gST->ConfigurationTable[i].VendorGuid, &gHobGuid)) {
                    gHobList = (VOID*)gST->ConfigurationTable[i].VendorTable;
                    break;
                }
            }
        }
    }
    return gHobList;
}

/**
 * Get Next HOB
 */
static VOID*
GetNextHob(
    IN UINT16 type,
    IN VOID* start
)
{
    EFI_PEI_HOB_POINTERS hob;
    hob.Raw = (UINT8*)start;

    while (!END_OF_HOB_LIST(hob)) {
        if (hob.Header->HobType == type) {
            return hob.Raw;
        }
        hob.Raw = GET_NEXT_HOB(hob);
    }

    return NULL;
}

/**
 * Get First HOB
 */
static VOID*
GetFirstHob(
    IN UINT16 type
)
{
    VOID* list = GetHobList();
    return GetNextHob(type, list);
}

/**
 * Get Next Guid HOB
 */
static VOID*
GetNextGuidHob(
    IN EFI_GUID* guid,
    IN VOID* start
)
{
    EFI_PEI_HOB_POINTERS guidHob;
    guidHob.Raw = (UINT8*)start;

    while ((guidHob.Raw = GetNextHob(EFI_HOB_TYPE_GUID_EXTENSION, guidHob.Raw)) != NULL) {
        if (CompareGuid(guid, &guidHob.Guid->Name)) {
            break;
        }
        guidHob.Raw = GET_NEXT_HOB(guidHob);
    }

    return guidHob.Raw;
}

/**
 * Get First Guid HOB
 */
static VOID*
GetFirstGuidHob(
    IN EFI_GUID* guid
)
{
    VOID* list = GetHobList();
    return GetNextGuidHob(guid, list);
}

/**
 * Check SMBIOS Entry Point validity (checksum)
 * EXACT COPY of negativespoofer's CheckEntry
 */
INTN
CheckEntry(
    IN SMBIOS_STRUCTURE_TABLE* entry
)
{
    if (!entry)
        return 0;

    // Safety: Check if entry looks valid (basic sanity check)
    // Verify anchor string "_SM_" at start
    if (entry->AnchorString[0] != '_' || 
        entry->AnchorString[1] != 'S' || 
        entry->AnchorString[2] != 'M' || 
        entry->AnchorString[3] != '_') {
        return 0;
    }
    
    // Verify entry point length is reasonable (should be 0x1F for SMBIOS 2.x)
    if (entry->EntryPointLength == 0 || entry->EntryPointLength > 64) {
        return 0;
    }

    CHAR8* pointer = (CHAR8*)entry;
    INTN checksum = 0;
    UINT8 length = entry->EntryPointLength;
    UINTN i;
    
    for (i = 0; i < (UINTN)length; i++) {
        checksum = checksum + (INTN)pointer[i];
    }

    return (checksum == 0);
}

/**
 * Find by Signature (DISABLED for safety - can cause page faults)
 */
static VOID*
FindBySignature(
    VOID
)
{
    // DISABLED - can cause page faults
    // Negativespoofer does this first, but we skip it for safety
    return NULL;
}

/**
 * Find by HOB
 * EXACT COPY of negativespoofer's FindByHob
 */
static VOID*
FindByHob(
    VOID
)
{
    EFI_PHYSICAL_ADDRESS* table;
    EFI_PEI_HOB_POINTERS guidHob;

    guidHob.Raw = (UINT8*)GetFirstGuidHob(&gSmbiosTableGuid);

    if (guidHob.Raw != NULL) {
        table = (EFI_PHYSICAL_ADDRESS*)GET_GUID_HOB_DATA(guidHob.Guid);
        if (table != NULL) {
            return (VOID*)table;
        }
    }

    guidHob.Raw = (UINT8*)GetFirstGuidHob(&gSmbios3TableGuid);

    if (guidHob.Raw != NULL) {
        table = (EFI_PHYSICAL_ADDRESS*)GET_GUID_HOB_DATA(guidHob.Guid);
        if (table != NULL) {
            return (VOID*)table;
        }
    }

    return NULL;
}

/**
 * Find by Config Table
 * EXACT COPY of negativespoofer's FindByConfig
 */
static VOID*
FindByConfig(
    VOID
)
{
    if (gST == NULL || gST->ConfigurationTable == NULL) {
        return NULL;
    }
    
    UINTN i;
    for (i = 0; i < (UINTN)gST->NumberOfTableEntries; i++) {
        if (CompareGuid(&gST->ConfigurationTable[i].VendorGuid, &gSmbiosTableGuid)) {
            // Negativespoofer returns VendorTable directly (it's already a pointer to entry point)
            return (VOID*)gST->ConfigurationTable[i].VendorTable;
        }
        if (CompareGuid(&gST->ConfigurationTable[i].VendorGuid, &gSmbios3TableGuid)) {
            return (VOID*)gST->ConfigurationTable[i].VendorTable;
        }
    }

    return NULL;
}

/**
 * Find SMBIOS Entry Point
 * EXACT COPY of negativespoofer's FindEntry
 * Note: Negativespoofer does NOT call CheckEntry here - it returns address directly
 */
SMBIOS_STRUCTURE_TABLE*
FindEntry(
    VOID
)
{
    SMBIOS_STRUCTURE_TABLE* address = NULL;
    
    // Order: Signature -> Config -> HOB (exact negativespoofer order)
    address = (SMBIOS_STRUCTURE_TABLE*)FindBySignature();
    if (address) {
        return address;
    }

    address = (SMBIOS_STRUCTURE_TABLE*)FindByConfig();
    if (address) {
        return address;
    }

    address = (SMBIOS_STRUCTURE_TABLE*)FindByHob();
    if (address) {
        return address;
    }

    return NULL;
}

