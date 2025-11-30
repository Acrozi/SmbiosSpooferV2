/*
 * EFI SMBIOS Spoofer Configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Uefi.h>

#define SPOOF_ENABLED          TRUE
#define SPOOF_SYSTEM_INFO       TRUE
#define USE_RANDOM_UUID         TRUE

#define USE_NVRAM_PERSISTENCE   1
#define NVRAM_VARIABLE_NAME     L"SmbiosSpoof"
#define FORCE_NEW_UUID          0

#define USE_DISK_PERSISTENCE    1
#define UUID_FILE_PATH          L"\\EFI\\SmbiosSpoofer\\uuid.dat"

#define SPOOF_SYSTEM_SERIAL     1
#define SPOOF_BIOS_SERIAL       0
#define SPOOF_BASEBOARD_SERIAL  1
#define SPOOF_BASEBOARD_MODEL   0
#define SPOOF_PROCESSOR_SERIAL  1

#endif

