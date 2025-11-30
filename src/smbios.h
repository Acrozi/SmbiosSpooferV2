/*
 * SMBIOS Structures and Functions
 */

#ifndef SMBIOS_H
#define SMBIOS_H

#include <Uefi.h>
#include <Protocol/Smbios.h>

// SMBIOS Type Constants
#define SMBIOS_TYPE_BIOS_INFORMATION                     0
#define SMBIOS_TYPE_SYSTEM_INFORMATION                   1
#define SMBIOS_TYPE_BASEBOARD_INFORMATION                 2
#define SMBIOS_TYPE_SYSTEM_ENCLOSURE                      3
#define SMBIOS_TYPE_PROCESSOR_INFORMATION                 4
#define SMBIOS_TYPE_END_OF_TABLE                         0x007F

// SMBIOS Entry Point Structure (32-bit)
typedef struct {
    CHAR8   AnchorString[4];        // "_SM_"
    UINT8   EntryPointLength;
    UINT8   MajorVersion;
    UINT8   MinorVersion;
    UINT16  MaxStructureSize;
    UINT8   EntryPointRevision;
    UINT8   FormattedArea[5];
    CHAR8   IntermediateAnchorString[5]; // "_DMI_"
    UINT8   IntermediateChecksum;
    UINT16  StructureTableLength;
    UINT32  StructureTableAddress;
    UINT16  NumberOfStructures;
    UINT8   BCDRevision;
} SMBIOS_STRUCTURE_TABLE;

// SMBIOS Header
typedef struct {
    UINT8   Type;
    UINT8   Length;
    UINT16  Handle;
} SMBIOS_HEADER;

// Type 0: BIOS Information
typedef struct {
    SMBIOS_HEADER  Header;
    UINT8          Vendor;           // String index
    UINT8          BiosVersion;       // String index
    UINT16         BiosStartingAddressSegment;
    UINT8          BiosReleaseDate;  // String index (we use this for BIOS Serial)
    UINT8          BiosRomSize;
    // ... more fields
} SMBIOS_TYPE0_BIOS_INFO;

// Type 1: System Information
typedef struct {
    SMBIOS_HEADER  Header;
    UINT8          Manufacturer;     // String index
    UINT8          ProductName;       // String index
    UINT8          Version;            // String index
    UINT8          SerialNumber;       // String index
    UINT8          UUID[16];          // UUID (we modify this)
    UINT8          WakeUpType;
    UINT8          SKUNumber;          // String index
    UINT8          Family;             // String index
    // ... more fields
} SMBIOS_TYPE1_SYSTEM_INFO;

// Type 2: Baseboard Information
typedef struct {
    SMBIOS_HEADER  Header;
    UINT8          Manufacturer;      // String index
    UINT8          ProductName;       // String index (we use this for Model)
    UINT8          Version;           // String index
    UINT8          SerialNumber;      // String index
    UINT8          AssetTag;          // String index
    UINT8          FeatureFlags;
    UINT8          LocationInChassis; // String index
    UINT16         ChassisHandle;
    UINT8          BoardType;
    // ... more fields
} SMBIOS_TYPE2_BASEBOARD_INFO;

// Type 4: Processor Information
typedef struct {
    SMBIOS_HEADER  Header;
    UINT8          SocketDesignation;  // String index
    UINT8          ProcessorType;
    UINT8          ProcessorFamily;
    UINT8          ProcessorManufacturer; // String index
    UINT64         ProcessorId;        // CPUID (hardware-nivå, kan inte spoofas)
    UINT8          ProcessorVersion;   // String index
    UINT8          Voltage;
    UINT16         ExternalClock;
    UINT16         MaxSpeed;
    UINT16         CurrentSpeed;
    UINT8          Status;
    UINT8          ProcessorUpgrade;
    UINT16         L1CacheHandle;
    UINT16         L2CacheHandle;
    UINT16         L3CacheHandle;
    UINT8          SerialNumber;       // String index (we modify this)
    UINT8          AssetTag;          // String index
    // ... more fields (SMBIOS 2.5+)
} SMBIOS_TYPE4_PROCESSOR_INFO;

// SMBIOS Structure Pointer (union for type-safe access)
// Note: Using custom name to avoid conflict with MdePkg's SMBIOS_STRUCTURE_POINTER
typedef union {
    UINT8*                      Raw;
    SMBIOS_HEADER*              Hdr;
    SMBIOS_TYPE0_BIOS_INFO*     Type0;
    SMBIOS_TYPE1_SYSTEM_INFO*   Type1;
    SMBIOS_TYPE2_BASEBOARD_INFO* Type2;
    SMBIOS_TYPE4_PROCESSOR_INFO* Type4;
} SMBIOS_STRUCTURE_POINTER_CUSTOM;

// String field type (UINT8 index)
typedef UINT8 SMBIOS_STRING;

// Function declarations
UINT16 TableLenght(SMBIOS_STRUCTURE_POINTER_CUSTOM table);
SMBIOS_STRUCTURE_POINTER_CUSTOM FindTableByType(SMBIOS_STRUCTURE_TABLE* entry, UINT8 type, UINTN index);
UINTN SpaceLength(CONST CHAR8* text, UINTN maxLength);
VOID EditString(SMBIOS_STRUCTURE_POINTER_CUSTOM table, SMBIOS_STRING* field, CONST CHAR8* buffer);

// Read SMBIOS string from structure
VOID ReadSmbiosString(SMBIOS_STRUCTURE_POINTER_CUSTOM table, SMBIOS_STRING fieldIndex, OUT CHAR16* output, IN UINTN maxLength);

// UUID and persistence functions
EFI_STATUS EfiModifySmbiosType1UUID(EFI_SMBIOS_PROTOCOL* SmbiosProtocol, UINT8* UUID);
EFI_STATUS EfiLoadSpoofFromNvram(UINT8* UUID, CHAR16* SystemSerial, CHAR16* BiosSerial, CHAR16* BaseboardSerial, CHAR16* BaseboardModel, CHAR16* ProcessorSerial);
EFI_STATUS EfiSaveSpoofToNvram(UINT8* UUID, CHAR16* SystemSerial, CHAR16* BiosSerial, CHAR16* BaseboardSerial, CHAR16* BaseboardModel, CHAR16* ProcessorSerial);
EFI_STATUS EfiLoadSpoofFromDisk(UINT8* UUID, CHAR16* SystemSerial, CHAR16* BiosSerial, CHAR16* BaseboardSerial, CHAR16* BaseboardModel, CHAR16* ProcessorSerial);
EFI_STATUS EfiSaveSpoofToDisk(UINT8* UUID, CHAR16* SystemSerial, CHAR16* BiosSerial, CHAR16* BaseboardSerial, CHAR16* BaseboardModel, CHAR16* ProcessorSerial);
VOID EfiGenerateRandomUUID(UINT8* UUID);
VOID EfiGenerateRandomSerial(CHAR16* Serial, UINTN MaxLength);
VOID EfiGenerateRandomSerialMatchingFormat(CHAR16* Serial, UINTN MaxLength, CONST CHAR16* OriginalSerial);

// Finder functions
SMBIOS_STRUCTURE_TABLE* FindEntry(VOID);
INTN CheckEntry(SMBIOS_STRUCTURE_TABLE* entry);

#endif // SMBIOS_H

