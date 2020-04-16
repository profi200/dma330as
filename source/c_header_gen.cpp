#include <cstdio>
#include "types.h"



int makeCHeader(const u8 *const buf, u32 size, const char *const path)
{
	FILE *fh = fopen(path, "wb");
	if(fh)
	{
		// TODO: Error checking.
		fprintf(fh, "#include <stdint.h>\n\nstatic const uint8_t program[%" PRIu32 "] =\n{\n\t", size);
		for(u32 i = 0; i < size - 1; i++)
		{
			fprintf(fh, "0x%02" PRIX8 ", ", buf[i]); // TODO: Newline each X bytes.
		}
		fprintf(fh, "0x%02" PRIX8 "\n};\n", buf[size - 1]);

		fclose(fh);
	}
	else
	{
		fprintf(stderr, "Failed to open '%s'.\n", path);
		return 1;
	}

	return 0;
}
