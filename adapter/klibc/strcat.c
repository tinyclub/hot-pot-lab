/*
 * strcat.c
 */

#include <string.h>

char * __attribute__((weak)) strcat(char *dst, const char *src)
{
	strcpy(strchr(dst, '\0'), src);
	return dst;
}
