#include <gbc_format.h>
#include <string.h>

void usage(const char* program_name)
{
	printf("Usage : %s [ -logo ] <rom_filename>\n", program_name);
	printf("\t-logo (optional) : print logo\n");
}

int main(int argc, char *argv[])
{
	GB* rom = NULL;
	int view_logo = 0;

	if (argc < 2)
	{
		usage(argv[0]);
		return 0;
	}

	// Check for logo option
	if (strcmp(argv[1], "-logo") == 1)
		view_logo = 1;

	// Find the right filename argument
	if (argc == 2)
		rom = gbc_open(argv[1]);
	else
		rom = gbc_open(argv[2]);

	view_logo = view_logo;

	// Read header file information
	gbc_read_header(rom);

	// Print header
	gbc_print_header(rom);

	// Very header content
	gbc_check_header(rom);

	// Close rom
	gbc_close(rom);

	return 0;
}
