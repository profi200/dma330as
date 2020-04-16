#include <cstring>
#include "utils.h"



const char* findChar(const char *str)
{
	while(*str != '\0' && (*str < '!' || *str > '~')) str++;

	if(*str == '\0') return nullptr;

	return str;
}

// Returns the index of the match or -1.
// If cmpSize is 0 strcmp() is used otherwise strncmp().
s32 checkStrList(const char *const list[], u32 lSize, u32 cmpSize, const char *const str)
{
	s32 res = -1;
	u32 i = 0;

	do
	{
		if(cmpSize)
		{
			if(strncmp(list[i], str, cmpSize) == 0)
			{
				res = i;
				break;
			}
		}
		else
		{
			if(strcmp(list[i], str) == 0)
			{
				res = i;
				break;
			}
		}

		i++;
	} while(i < lSize);

	return res;
}

// Assumes no newline at the end of the string.
/*const char* findWhitespace(const char *str)
{
	while(*str != '\0' && *str >= '!' && *str <= '~') str++;

	if(*str == '\0') return nullptr;

	return str;
}

// Also removes newlines.
void stripComment(char *line)
{
	while(*line != '\0' && *line != '#' && *line != '\n') line++;

	*line = '\0';
}*/
