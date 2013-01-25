#include <gbc_format.h>
#include <stdlib.h>
#include <string.h>



const uint8_t logo_official[0x30] = 
{
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
};


/**
 * Open filename and init GB structure
 **/
GB* gbc_open(const char* filename)
{
    GB* rom = NULL;
    
    // Allocate memory for rom
    rom = malloc(sizeof(GB));
    CHECK_NULL(rom);

    // Open file
    if ((rom->stream = fopen(filename, "r")) == NULL)
        FATAL_ERROR("Unable to open file %s.\n", filename);

    // Save filename
    rom->filename = malloc((strlen(filename) + 1) * sizeof(char));
    CHECK_NULL(rom->filename);
    strcpy(rom->filename, filename);

    // Init struct
    rom->header = NULL;

    return rom;
}

/**
 * Close and free GB struct
 **/
void gbc_close(GB* rom)
{
    fclose(rom->stream);
    free(rom->filename);
    if (rom->header != NULL)
        free(rom->header);

    free(rom);
}

/**
 * Read header struct from file
 **/
void gbc_read_header(GB *rom)
{
    // Allocate memory for header
    rom->header = malloc(sizeof(GB_Header));
    CHECK_NULL(rom->header);

    // Go to header
    fseek(rom->stream, HEADER_START, SEEK_SET);

    // Read header
    if (fread(rom->header, sizeof(GB_Header), 1, rom->stream) != 1)
        FATAL_ERROR("Unable to read header information");
}

/**
 * Check header content
 **/
void gbc_check_header(GB *rom)
{
    uint8_t byte;
    uint16_t checksum;
    int i = 0;

    // First check : logo content
    fseek(rom->stream, LOGO_START, SEEK_SET);
    byte = fgetc(rom->stream);
    while (!feof(rom->stream) && i < 0x30)
    {
        // Check value
        if (byte != logo_official[i])
            FATAL_ERROR("Failed to check logo on octet %u (%X should be %X)", i, byte, logo_official[i]);
        i++;
        byte = fgetc(rom->stream);
    }

    // Check if we read whole logo
    if (i != 0x30)
        FATAL_ERROR("Logo seams to be shortened !");

    // Second : header checksum
    byte = 0;
    fseek(rom->stream, LOGO_START + 0x30, SEEK_SET);

    for (i = LOGO_START + 0x30; i < HEADER_CHECKSUM_START; i++)
    {
        byte = byte - fgetc(rom->stream) - 1;
    }

    if (byte != rom->header->header_checksum)
        FATAL_ERROR("Header checksum doesn't match (%X should be %X)", byte, rom->header->header_checksum);

    // Third : Global Checksum
    checksum = 0;
    fseek(rom->stream, 0x0, SEEK_SET);
    byte = fgetc(rom->stream);
    i = 0;

    while (!feof(rom->stream))
    {
        // Not the two checksum bytes
        if (i != 0x14E && i != 0x14F)
            checksum += byte;

        byte = fgetc(rom->stream);
    }

    // Just warning because Gameboy doesn't really verify this checksum
    if (checksum != rom->header->global_checksum)
        WARNING("Global checksum doesn't match (not really a problem)");
}

/**
 * Print header content 
 **/
void gbc_print_header(GB *rom)
{
    printf("Entry Point : %.2X %.2X %.2X %.2X\n",
           rom->header->entry_point[0],
           rom->header->entry_point[1],
           rom->header->entry_point[2],
           rom->header->entry_point[3]);

    printf("Title       : %s\n", (char *)&(rom->header->title));
    printf("Man. Code   : %c%c%c%c\n", 
           rom->header->manufacturer_code[0],
           rom->header->manufacturer_code[1],
           rom->header->manufacturer_code[2],
           rom->header->manufacturer_code[3]
        );
    printf("CGB Flag    : 0x%.2X\n", rom->header->CGB_flag);
    printf("NewLicCode  : %c%c\n", rom->header->new_licensee_code[0], rom->header->new_licensee_code[1]);
    printf("SGB Flag    : 0x%.2X\n", rom->header->sgb_flag);
    printf("Type        : 0x%.2X\n", rom->header->type);
    printf("RomSize     : 0x%.2X\n", rom->header->rom_size);
    printf("RamSize     : 0x%.2X\n", rom->header->ram_size);
    printf("Dest. Code  : 0x%.2X\n", rom->header->destination);
    printf("OldLicCode  : 0x%.2X\n", rom->header->old_licensee_code);
    printf("Version     : 0x%.2X\n", rom->header->version);
    printf("Header Chks : 0x%.2X\n", rom->header->header_checksum);
    printf("Global Chks : 0x%.4X\n", rom->header->global_checksum);
}
