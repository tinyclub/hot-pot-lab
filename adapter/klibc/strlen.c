/*
 * strlen()
 */

#include <string.h>

size_t __attribute__((weak)) strlen(const char *s)
{
	const char *ss = s;
	while (*ss)
		ss++;
	return ss - s;
}
