#include <gbc_format.h>
#include <stdlib.h>
#include <string.h>

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
