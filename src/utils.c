/*
 * Random Number Generation V2
 * EXACT COPY of negativespoofer's utils.c converted to EDK2
 */

#include "smbios.h"
#include <Library/UefiRuntimeServicesTableLib.h>

static INTN gLastRandom = 0;

/**
 * Random Number Generator
 * EXACT COPY of negativespoofer's RandomNumber
 */
INTN
RandomNumber(
    IN INTN l,
    IN INTN h
)
{
    EFI_TIME time;
    EFI_TIME_CAPABILITIES cap;
    
    if (gRT != NULL && gRT->GetTime != NULL) {
        gRT->GetTime(&time, &cap); // hopefully this does not fail
    } else {
        return l; // Fallback
    }

    if (gLastRandom == 0) {
        gLastRandom = time.Day + time.Hour + time.Minute + time.Second + time.Nanosecond;
    }
    gLastRandom += time.Minute;
    
    INTN num = gLastRandom % (h - l + 1);

    return num + l;
}

/**
 * Generate Random Text
 * EXACT COPY of negativespoofer's RandomText
 */
VOID
RandomText(
    OUT CHAR8* s,
    IN INTN len
)
{
    INTN i;
    for (i = 0; i < len; i++) {
        s[i] = (CHAR8)RandomNumber(49, 90);
    }

    s[len] = 0;
}

