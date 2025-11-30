/*
 * SMBIOS Functions
 */

#include "smbios.h"
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>

/**
 * Calculate total length of SMBIOS structure (including string table)
 * EXACT COPY of negativespoofer's TableLenght (with typo)
 */
UINT16
TableLenght(
    IN SMBIOS_STRUCTURE_POINTER_CUSTOM table
)
{
    // EXACT COPY of negativespoofer's TableLenght - no safety checks
    CHAR8* pointer = (CHAR8*)(table.Raw + table.Hdr->Length);
    while ((*pointer != 0) || (*(pointer + 1) != 0)) {
        pointer++;
    }
    
    return (UINT16)((UINTN)pointer - (UINTN)table.Raw + 2);
}

/**
 * Find SMBIOS structure by type in the table
 * EXACT COPY of negativespoofer's FindTableByType
 */
SMBIOS_STRUCTURE_POINTER_CUSTOM
FindTableByType(
    IN SMBIOS_STRUCTURE_TABLE* entry,
    IN UINT8 type,
    IN UINTN index
)
{
    SMBIOS_STRUCTURE_POINTER_CUSTOM smbiosTable;
    smbiosTable.Raw = NULL;
    
    if (entry == NULL) {
        return smbiosTable;
    }
    
    // EXACT COPY of negativespoofer: No safety checks, just use TableAddress
    // Note: In negativespoofer it's "TableAddress", in EDK2 it's "StructureTableAddress"
    smbiosTable.Raw = (UINT8*)((UINTN)entry->StructureTableAddress);
    // EXACT COPY of negativespoofer: Simple check, no extra safety
    if (!smbiosTable.Raw) {
        return smbiosTable;
    }

    UINTN typeIndex = 0;
    // EXACT COPY: Negativespoofer has no max iterations limit
    while ((typeIndex != index) || (smbiosTable.Hdr->Type != type)) {
        if (smbiosTable.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
            smbiosTable.Raw = (UINT8*)NULL;
            return smbiosTable;
        }

        if (smbiosTable.Hdr->Type == type) {
            typeIndex++;
        }

        smbiosTable.Raw = (UINT8*)(smbiosTable.Raw + TableLenght(smbiosTable));
    }

    return smbiosTable;
}

/**
 * Calculate space length of string (trimmed)
 * EXACT COPY of negativespoofer's SpaceLength
 */
UINTN
SpaceLength(
    IN CONST CHAR8* text,
    IN UINTN maxLength
)
{
    UINTN lenght = 0;
    CONST CHAR8* ba;

    if (maxLength > 0) {
        for (lenght = 0; lenght < maxLength; lenght++) {
            if (text[lenght] == 0) {
                break;
            }
        }

        ba = &text[lenght - 1];

        while ((lenght != 0) && ((*ba == ' ') || (*ba == 0))) {
            ba--;
            lenght--;
        }
    } else {
        ba = text;
        while (*ba != 0) {
            ba++;
            lenght++;
        }
    }

    return lenght;
}

/**
 * Edit string directly in SMBIOS table (in-place editing)
 * EXACT COPY of negativespoofer's EditString
 */
VOID
EditString(
    IN SMBIOS_STRUCTURE_POINTER_CUSTOM table,
    IN SMBIOS_STRING* field,
    IN CONST CHAR8* buffer
)
{
    if (!table.Raw || !buffer || !field)
        return;

    UINT8 index = 1;
    CHAR8* astr = (CHAR8*)(table.Raw + table.Hdr->Length);
    while (index != *field) {
        if (*astr) {
            index++;
        }

        while (*astr != 0)
            astr++;
        astr++;

        if (*astr == 0) {
            if (*field == 0) {
                astr[1] = 0;
            }

            *field = index;

            if (index == 1) {
                astr--;
            }
            break;
        }
    }

    UINTN astrLength = SpaceLength(astr, 0);
    UINTN bstrLength = SpaceLength(buffer, 256);

    // Debug output
    // Print(L"[DEBUG] EditString: astrLength=%d, bstrLength=%d\n", astrLength, bstrLength);

    // If new string is shorter than original, we can't fit it without resizing
    // But we can pad with spaces or truncate the original
    if (bstrLength < astrLength) {
        // New string is shorter - pad with spaces to match original length
        UINTN i;
        for (i = 0; i < bstrLength && i < 255; i++) {
            astr[i] = buffer[i];
        }
        // Pad rest with spaces (if original had spaces) or nulls
        for (; i < astrLength - 1 && i < 255; i++) {
            astr[i] = ' '; // Pad with space
        }
        // Ensure null terminator
        if (astrLength > 1) {
            astr[astrLength - 1] = 0;
        }
        return;
    }

    // Safety check: ensure we have valid lengths
    if (astrLength == 0) {
        return;
    }
    
    // I am lazy piece of shit and I am not implementing some string resizing
    // Copy new string (astrLength - 1 bytes, preserving NULL terminator)
    // This means we can only copy up to the original string's length
    UINTN copyLength = (astrLength > 1) ? (astrLength - 1) : 0;
    if (copyLength > 0) {
        // Copy up to original length, but ensure we don't exceed buffer length
        if (copyLength > bstrLength) {
            copyLength = bstrLength;
        }
        CopyMem(astr, buffer, copyLength);
        // Ensure null terminator
        if (copyLength < astrLength - 1) {
            astr[copyLength] = 0;
        }
    }
}

/**
 * Read SMBIOS string from structure
 * Reads the string at the given field index and converts to CHAR16
 */
VOID
ReadSmbiosString(
    IN SMBIOS_STRUCTURE_POINTER_CUSTOM table,
    IN SMBIOS_STRING fieldIndex,
    OUT CHAR16* output,
    IN UINTN maxLength
)
{
    if (!table.Raw || !output || maxLength == 0 || fieldIndex == 0) {
        if (output && maxLength > 0) {
            output[0] = 0;
        }
        return;
    }
    
    UINT8 index = 1;
    CHAR8* astr = (CHAR8*)(table.Raw + table.Hdr->Length);
    
    // Find the string at the given index
    while (index != fieldIndex) {
        if (*astr) {
            index++;
        }
        
        while (*astr != 0) {
            astr++;
        }
        astr++;
        
        // Check if we've reached the end of strings
        if (*astr == 0 && *(astr + 1) == 0) {
            // String not found
            output[0] = 0;
            return;
        }
    }
    
    // Convert CHAR8 to CHAR16
    UINTN i;
    for (i = 0; i < maxLength - 1 && astr[i] != 0; i++) {
        output[i] = (CHAR16)astr[i];
    }
    output[i] = 0;
}

