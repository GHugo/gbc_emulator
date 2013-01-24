#ifndef __GBC_FORMAT_H__
#define __GBC_FORMAT_H__

/** Different types **/
#include <stdint.h>
#include <stdio.h>

/** Error Handling **/
#define FATAL_ERROR(format, ...) \
{                                                                       \
    fprintf(stderr, "[Error : %s:%u] " # format, __FILE__, __LINE__, ## __VA_ARGS__); \
    exit(-1);                                                           \
}

#define WARNING(format, ...) \
{                                                                       \
    fprintf(stdout, "[Warning : " ## __FILE__ ## __LINE__ ## "] " ##    \
            format, ## __VAR_ARGS__);                                   \
}

#define CHECK_NULL(x) if (x == NULL) FATAL_ERROR("Unable to allocate memory. Exciting...\n")

/** Main struct for GB handling **/
// See http://nocash.emubase.de/pandocs.htm#thecartridgeheader
typedef enum
{
    ROM_ONLY = 0x00,
    MBC1 = 0x01,
    MBC1_RAM = 0x02,
    MBC1_RAM_BATTERY = 0x03,
    MBC2 = 0x05,
    MBC2_BATTERY = 0x06,
    ROM_RAM = 0x08,
    ROM_RAM_BATTERY = 0x09,
    MMM01 = 0x0B,
    MMM01_RAM = 0x0C,
    MMM01_RAM_BATTERY = 0x0D,
    MBC3_TIMER_BATTERY = 0x0F,
    MBC3_TIMER_RAM_BATTERY = 0x10,
    MBC3 = 0x11,
    MBC3_RAM = 0x12,
    MBC3_RAM_BATTERY = 0x13,
    MBC4 = 0x15,
    MBC4_RAM = 0x16,
    MBC4_RAM_BATTERY = 0x17,
    MBC5 = 0x19,
    MBC5_RAM = 0x1A,
    MBC5_RAM_BATTERY = 0x1B,
    MBC5_RUMBLE = 0x1C,
    MBC5_RUMBLE_RAM = 0x1D,
    MBC5_RUMBLE_RAM_BATTERY = 0x1E,
    POCKET_CAMERA = 0xFC,
    BANDAI_TAMA5 = 0xFD,
    HuC3 = 0xFE,
    HuC1_RAM_BATTERY = 0xFF,
} CartridgeType;

typedef enum 
{
    S32KByte = 0x00,
    S64KByte = 0x01,
    S128KByte = 0x02,
    S256KByte = 0x03,
    S512KByte = 0x04,
    S1MByte = 0x05,
    S2MByte = 0x06,
    S4MByte = 0x07,
    S1_1MByte = 0x52,
    S1_2MByte = 0x53,
    S1_5MByte = 0x54,
} RomSize;

typedef enum
{
    None = 0x00,
    S2KBytes = 0x01,
    S8KBytes = 0x02,
    S32KBytes = 0x03,
} RamSize;

typedef enum
{
    Japanse = 0x00,
    Non_Japanese = 0x01
} DestinationCode;

typedef struct
{
    uint8_t entry_point[0x4];
    uint8_t logo[0x30];
    char *title;
    char manufacturer_code[0x4];
    uint8_t CGB_flag;
    char new_licensee_code[2];
    uint8_t sgb_flag;
    CartridgeType type;
    RomSize rom_size;
    RamSize ram_size;
    DestinationCode destination;
    uint8_t old_licesee_code;
    uint8_t version;
    uint8_t header_checksum;
    uint8_t global_checksum;
} GB_Header;
    
    
typedef struct
{
    /* File oriented */
    FILE* stream;
    char* filename;

    /* GB/GBC Oriented */
    GB_Header* header;
} GB;

/** Header Specification **/


/** Managing open/close of GBC file **/

/* Open and init GB structure */
GB* gbc_open(const char* filename);

/* Close file and free memory */
void gbc_close(GB* rom);


#endif // __GBC_FORMAT_H__
