/*
 * strpbrk
 */

#include <string.h>

#include "strxspn.h"

char * __attribute__((weak)) strpbrk(const char *s, const char *accept)
{
	const char *ss = s + __strxspn(s, accept, 1);

	return *ss ? (char *)ss : NULL;
}
