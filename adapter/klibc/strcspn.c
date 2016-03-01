/*
 * strcspn
 */

#include <string.h>

#include "strxspn.h"

size_t __attribute__((weak)) strcspn(const char *s, const char *reject)
{
	return __strxspn(s, reject, 1);
}
