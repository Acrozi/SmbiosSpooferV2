/*
 * SMBIOS Spoofer - Main Entry Point
 */

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/Smbios.h>
#include "smbios.h"
#include "Config.h"

static VOID
Delay(
    IN UINTN Microseconds
)
{
    if (gBS != NULL && gBS->Stall != NULL) {
        gBS->Stall(Microseconds);
    }
}


extern VOID SetSpoofedUUID(UINT8* UUID);
extern VOID SetSpoofedSerials(CHAR16* SystemSerial, CHAR16* BiosSerial, CHAR16* BaseboardSerial, CHAR16* BaseboardModel, CHAR16* ProcessorSerial);
extern VOID PatchAll(SMBIOS_STRUCTURE_TABLE* entry);

static VOID
PrintUUID(
    IN UINT8* UUID
)
{
    if (UUID == NULL) {
        Print(L"NULL");
        return;
    }
    
    Print(L"%02X%02X%02X%02X-", UUID[0], UUID[1], UUID[2], UUID[3]);
    Print(L"%02X%02X-", UUID[4], UUID[5]);
    Print(L"%02X%02X-", UUID[6], UUID[7]);
    Print(L"%02X%02X-", UUID[8], UUID[9]);
    Print(L"%02X%02X%02X%02X%02X%02X", UUID[10], UUID[11], UUID[12], UUID[13], UUID[14], UUID[15]);
}

EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE* SystemTable
)
{
    EFI_STATUS status;
    EFI_SMBIOS_PROTOCOL* smbiosProtocol = NULL;
    SMBIOS_STRUCTURE_TABLE* smbiosEntry = NULL;
    UINT8 uuid[16];
    CHAR16 systemSerial[64];
    CHAR16 biosSerial[64];
    CHAR16 baseboardSerial[64];
    CHAR16 baseboardModel[64];
    CHAR16 processorSerial[64];
    BOOLEAN loadedFromStorage = FALSE;
    
    if (gST == NULL || gST->ConOut == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    ZeroMem(uuid, sizeof(uuid));
    ZeroMem(systemSerial, sizeof(systemSerial));
    ZeroMem(biosSerial, sizeof(biosSerial));
    ZeroMem(baseboardSerial, sizeof(baseboardSerial));
    ZeroMem(baseboardModel, sizeof(baseboardModel));
    ZeroMem(processorSerial, sizeof(processorSerial));
    
    gST->ConOut->ClearScreen(gST->ConOut);
    
    Print(L"\n");
    Print(L"========================================\n");
    Print(L"  EFI SMBIOS Spoofer V2\n");
    Print(L"  Developed by Acrozi\n");
    Print(L"  https://github.com/Acrozi\n");
    Print(L"========================================\n");
    Print(L"\n");
    
    Print(L"[WORK] Searching for SMBIOS table entry...\n");
    Delay(500000);
    smbiosEntry = FindEntry();
    if (!smbiosEntry) {
        Print(L"[FAIL] Failed to locate SMBIOS table entry\n");
        Print(L"[FAIL] Trying alternative methods...\n");
        Print(L"\n");
        Print(L"Press Enter to continue booting...\n");
        if (gST != NULL && gST->ConIn != NULL) {
            gST->ConIn->ReadKeyStroke(gST->ConIn, NULL);
        }
        return EFI_NOT_FOUND;
    }
    
    Print(L"[INFO] SMBIOS table entry found on 0x%016lx\n", (UINT64)smbiosEntry->StructureTableAddress);
    Delay(500000);
    
    status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID**)&smbiosProtocol);
    if (EFI_ERROR(status)) {
        Print(L"[WARN] Could not locate SMBIOS protocol (UUID spoofing disabled)\n");
        Delay(500000);
        smbiosProtocol = NULL;
    } else {
        Delay(500000);
    }
    
    #if defined(USE_NVRAM_PERSISTENCE) && USE_NVRAM_PERSISTENCE
    status = EfiLoadSpoofFromNvram(uuid, systemSerial, biosSerial, baseboardSerial, baseboardModel, processorSerial);
    if (!EFI_ERROR(status)) {
        Print(L"[NVRAM] Successfully loaded spoofed values from NVRAM\n");
        Delay(500000);
        loadedFromStorage = TRUE;
    }
    #endif
    
    if (!loadedFromStorage) {
        #if defined(USE_DISK_PERSISTENCE) && USE_DISK_PERSISTENCE
        status = EfiLoadSpoofFromDisk(uuid, systemSerial, biosSerial, baseboardSerial, baseboardModel, processorSerial);
        if (!EFI_ERROR(status)) {
            Print(L"[DISK] Successfully loaded spoofed values from disk\n");
            Delay(500000);
            loadedFromStorage = TRUE;
        }
        #endif
    }
    
    CHAR16 originalSystemSerial[64];
    CHAR16 originalBiosSerial[64];
    CHAR16 originalBaseboardSerial[64];
    CHAR16 originalBaseboardModel[64];
    CHAR16 originalProcessorSerial[64];
    UINT8 originalUUID[16];
    
    ZeroMem(originalSystemSerial, sizeof(originalSystemSerial));
    ZeroMem(originalBiosSerial, sizeof(originalBiosSerial));
    ZeroMem(originalBaseboardSerial, sizeof(originalBaseboardSerial));
    ZeroMem(originalBaseboardModel, sizeof(originalBaseboardModel));
    ZeroMem(originalProcessorSerial, sizeof(originalProcessorSerial));
    ZeroMem(originalUUID, sizeof(originalUUID));
    
    if (smbiosEntry != NULL) {
        SMBIOS_STRUCTURE_POINTER_CUSTOM table1 = FindTableByType(smbiosEntry, SMBIOS_TYPE_SYSTEM_INFORMATION, 0);
        if (table1.Raw != NULL && table1.Type1 != NULL) {
            CopyMem(originalUUID, table1.Type1->UUID, 16);
            
            if (table1.Type1->SerialNumber != 0) {
                ReadSmbiosString(table1, table1.Type1->SerialNumber, originalSystemSerial, 64);
                if (StrStr(originalSystemSerial, L"DEFAULT") != NULL || 
                    StrStr(originalSystemSerial, L"Default") != NULL ||
                    StrStr(originalSystemSerial, L"To be filled") != NULL) {
                    Print(L"[INFO] System Serial is a placeholder - will use default format\n");
                }
            }
        }
        
        originalBiosSerial[0] = 0;
        
        SMBIOS_STRUCTURE_POINTER_CUSTOM table2 = FindTableByType(smbiosEntry, SMBIOS_TYPE_BASEBOARD_INFORMATION, 0);
        if (table2.Raw != NULL && table2.Type2 != NULL) {
            if (table2.Type2->SerialNumber != 0) {
                ReadSmbiosString(table2, table2.Type2->SerialNumber, originalBaseboardSerial, 64);
            }
            
            if (table2.Type2->ProductName != 0) {
                ReadSmbiosString(table2, table2.Type2->ProductName, originalBaseboardModel, 64);
                CopyMem(baseboardModel, originalBaseboardModel, sizeof(originalBaseboardModel));
            }
        }
        
        SMBIOS_STRUCTURE_POINTER_CUSTOM table4 = FindTableByType(smbiosEntry, SMBIOS_TYPE_PROCESSOR_INFORMATION, 0);
        if (table4.Raw != NULL && table4.Type4 != NULL && table4.Type4->SerialNumber != 0) {
            ReadSmbiosString(table4, table4.Type4->SerialNumber, originalProcessorSerial, 64);
            if (StrStr(originalProcessorSerial, L"DEFAULT") != NULL || 
                StrStr(originalProcessorSerial, L"Default") != NULL ||
                StrStr(originalProcessorSerial, L"To be filled") != NULL) {
                Print(L"[INFO] Processor Serial is a placeholder - will use default format\n");
            }
        }
    }
    
    if (!loadedFromStorage) {
        Print(L"[UUID] Generating new UUID and serials...\n");
        Delay(500000);
        EfiGenerateRandomUUID(uuid);
        
        EfiGenerateRandomSerialMatchingFormat(systemSerial, 64, originalSystemSerial);
        biosSerial[0] = 0;
        EfiGenerateRandomSerialMatchingFormat(baseboardSerial, 64, originalBaseboardSerial);
        #if defined(SPOOF_PROCESSOR_SERIAL) && SPOOF_PROCESSOR_SERIAL
        EfiGenerateRandomSerialMatchingFormat(processorSerial, 64, originalProcessorSerial);
        #else
        processorSerial[0] = 0;
        #endif
        
        baseboardModel[0] = 0;
    } else {
        biosSerial[0] = 0;
    }
    Delay(500000);
    
    SetSpoofedUUID(uuid);
    SetSpoofedSerials(systemSerial, biosSerial, baseboardSerial, NULL, processorSerial);
    
    Print(L"\n");
    Print(L"========================================\n");
    Print(L"  Applying SMBIOS Spoofs\n");
    Print(L"========================================\n");
    
    if (smbiosEntry != NULL) {
        PatchAll(smbiosEntry);
    } else {
        Print(L"[FAIL] Cannot patch - entry is NULL\n");
    }
    
    Print(L"========================================\n");
    Print(L"  SMBIOS Spoofing Completed\n");
    Print(L"========================================\n");
    Delay(500000);
    
    if (smbiosProtocol != NULL && SPOOF_SYSTEM_INFO) {
        Print(L"[UUID] Modifying UUID via SMBIOS Protocol...\n");
        Delay(500000);
        status = EfiModifySmbiosType1UUID(smbiosProtocol, uuid);
        if (!EFI_ERROR(status)) {
            Print(L"[UUID] UUID modified successfully\n");
            Delay(500000);
        } else {
            Print(L"[UUID] UUID modification failed (status: %r)\n", status);
            Delay(500000);
        }
    }
    
    if (!loadedFromStorage) {
        #if defined(USE_NVRAM_PERSISTENCE) && USE_NVRAM_PERSISTENCE
        status = EfiSaveSpoofToNvram(uuid, systemSerial, biosSerial, baseboardSerial, baseboardModel, processorSerial);
        if (!EFI_ERROR(status)) {
            Print(L"[NVRAM] Successfully saved to NVRAM\n");
            Delay(500000);
        } else {
            Print(L"[NVRAM] Failed to save (status: %r)\n", status);
            Print(L"[NVRAM] Variable name: SmbiosSpoof\n");
            Print(L"[NVRAM] GUID: 8BE4DF61-93CA-11D2-AA0D-00E098032B8C\n");
            Print(L"[NVRAM] Try checking in RU.efi if variable exists\n");
            Delay(500000);
        }
        #endif
        
        #if defined(USE_DISK_PERSISTENCE) && USE_DISK_PERSISTENCE
        status = EfiSaveSpoofToDisk(uuid, systemSerial, biosSerial, baseboardSerial, baseboardModel, processorSerial);
        if (!EFI_ERROR(status)) {
            Print(L"[DISK] Successfully saved to disk\n");
            Delay(500000);
        }
        #endif
    }
    
    Print(L"\n");
    Print(L"========================================\n");
    Print(L"  ORIGINAL VALUES\n");
    Print(L"========================================\n");
    Print(L"\n");
    
    Print(L"UUID: ");
    PrintUUID(originalUUID);
    Print(L"\n");
    Delay(500000);
    
    if (originalSystemSerial[0] != 0) {
        Print(L"System Serial: %s\n", originalSystemSerial);
        Delay(500000);
    }
    
    if (originalBaseboardSerial[0] != 0) {
        Print(L"Baseboard Serial: %s\n", originalBaseboardSerial);
        Delay(500000);
    }
    
    if (originalBaseboardModel[0] != 0) {
        Print(L"Baseboard Model: %s\n", originalBaseboardModel);
        Delay(500000);
    }
    
    if (originalProcessorSerial[0] != 0) {
        Print(L"Processor Serial: %s\n", originalProcessorSerial);
        Delay(500000);
    }
    
    Print(L"\n");
    Print(L"========================================\n");
    Print(L"  SMBIOS SPOOF SUMMARY\n");
    Print(L"========================================\n");
    Print(L"\n");
    
    Print(L"UUID: ");
    PrintUUID(uuid);
    Print(L" %s\n", loadedFromStorage ? L"(loaded)" : L"(new)");
    Delay(500000);
    Print(L"\n");
    
    #if defined(SPOOF_SYSTEM_SERIAL) && SPOOF_SYSTEM_SERIAL
    if (systemSerial[0] != 0) {
        Print(L"System Serial: %s %s\n", systemSerial, loadedFromStorage ? L"(loaded)" : L"(new)");
        Delay(500000);
    }
    #endif
    
    #if defined(SPOOF_BASEBOARD_SERIAL) && SPOOF_BASEBOARD_SERIAL
    if (baseboardSerial[0] != 0) {
        Print(L"Baseboard Serial: %s %s\n", baseboardSerial, loadedFromStorage ? L"(loaded)" : L"(new)");
        Delay(500000);
    }
    #endif
    
    if (baseboardModel[0] != 0) {
        Print(L"Baseboard Model: %s (static)\n", baseboardModel);
        Delay(500000);
    }
    
    if (processorSerial[0] != 0) {
        Print(L"Processor Serial: %s %s\n", processorSerial, loadedFromStorage ? L"(loaded)" : L"(new)");
        Delay(500000);
    }
    
    Print(L"\n");
    Print(L"========================================\n");
    Print(L"\n");
    Print(L"All spoofs applied successfully!\n");
    Print(L"\n");
    Print(L"Press 'R' to reset and generate new values\n");
    Print(L"Press any other key to continue booting...\n");
    
    if (gST != NULL && gST->ConIn != NULL) {
        EFI_INPUT_KEY key;
        UINTN index;
        
        while (TRUE) {
            gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &index);
            gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
            
            if (key.UnicodeChar == L'R' || key.UnicodeChar == L'r') {
                Print(L"\n");
                Print(L"[RESET] Generating new values...\n");
                Print(L"\n");
                
                EfiGenerateRandomUUID(uuid);
                EfiGenerateRandomSerialMatchingFormat(systemSerial, 64, originalSystemSerial);
                biosSerial[0] = 0;
                EfiGenerateRandomSerialMatchingFormat(baseboardSerial, 64, originalBaseboardSerial);
                EfiGenerateRandomSerialMatchingFormat(processorSerial, 64, originalProcessorSerial);
                
                if (smbiosEntry != NULL) {
                    SMBIOS_STRUCTURE_POINTER_CUSTOM table = FindTableByType(smbiosEntry, SMBIOS_TYPE_BASEBOARD_INFORMATION, 0);
                    if (table.Raw != NULL && table.Type2 != NULL && table.Type2->ProductName != 0) {
                        ReadSmbiosString(table, table.Type2->ProductName, baseboardModel, 64);
                    } else {
                        baseboardModel[0] = 0;
                    }
                } else {
                    baseboardModel[0] = 0;
                }
                
                SetSpoofedUUID(uuid);
                SetSpoofedSerials(systemSerial, biosSerial, baseboardSerial, NULL, processorSerial);
                
                if (smbiosEntry != NULL) {
                    PatchAll(smbiosEntry);
                }
                
                if (smbiosProtocol != NULL && SPOOF_SYSTEM_INFO) {
                    EfiModifySmbiosType1UUID(smbiosProtocol, uuid);
                }
                #if defined(USE_NVRAM_PERSISTENCE) && USE_NVRAM_PERSISTENCE
                status = EfiSaveSpoofToNvram(uuid, systemSerial, biosSerial, baseboardSerial, baseboardModel, processorSerial);
                if (!EFI_ERROR(status)) {
                    Print(L"[NVRAM] New values saved to NVRAM\n");
                }
                #endif
                
                #if defined(USE_DISK_PERSISTENCE) && USE_DISK_PERSISTENCE
                status = EfiSaveSpoofToDisk(uuid, systemSerial, biosSerial, baseboardSerial, baseboardModel, processorSerial);
                if (!EFI_ERROR(status)) {
                    Print(L"[DISK] New values saved to disk (overridden)\n");
                }
                #endif
                
                Print(L"\n");
                Print(L"========================================\n");
                Print(L"  NEW SMBIOS SPOOF SUMMARY\n");
                Print(L"========================================\n");
                Print(L"\n");
                
                Print(L"UUID: ");
                PrintUUID(uuid);
                Print(L" (new)\n");
                Delay(500000);
                Print(L"\n");
                
                #if defined(SPOOF_SYSTEM_SERIAL) && SPOOF_SYSTEM_SERIAL
                if (systemSerial[0] != 0) {
                    Print(L"System Serial: %s (new)\n", systemSerial);
                    Delay(500000);
                }
                #endif
                
                #if defined(SPOOF_BASEBOARD_SERIAL) && SPOOF_BASEBOARD_SERIAL
                if (baseboardSerial[0] != 0) {
                    Print(L"Baseboard Serial: %s (new)\n", baseboardSerial);
                    Delay(500000);
                }
                #endif
                
                if (baseboardModel[0] != 0) {
                    Print(L"Baseboard Model: %s (static)\n", baseboardModel);
                    Delay(500000);
                }
                
                if (processorSerial[0] != 0) {
                    Print(L"Processor Serial: %s (new)\n", processorSerial);
                    Delay(500000);
                }
                
                Print(L"\n");
                Print(L"========================================\n");
                Print(L"\n");
                Print(L"New values generated and applied!\n");
                Print(L"\n");
                Print(L"Press 'R' to reset again\n");
                Print(L"Press any other key to continue booting...\n");
            } else {
                break;
            }
        }
    }
    
    return EFI_SUCCESS;
}

