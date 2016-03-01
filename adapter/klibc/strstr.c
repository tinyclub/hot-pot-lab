/*
 * strstr.c
 */

#include <string.h>

char * __attribute__((weak)) strstr(const char *haystack, const char *needle)
{
	return (char *)memmem(haystack, strlen(haystack), needle,
			      strlen(needle));
}
