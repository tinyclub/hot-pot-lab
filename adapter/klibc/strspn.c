/*
 * strspn
 */

#include <string.h>

#include "strxspn.h"

size_t __attribute__((weak)) strspn(const char *s, const char *accept)
{
	return __strxspn(s, accept, 0);
}
