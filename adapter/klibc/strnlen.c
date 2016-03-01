/*
 * strnlen()
 */

#include <string.h>

size_t __attribute__((weak)) strnlen(const char *s, size_t maxlen)
{
	const char *ss = s;

	/* Important: the maxlen test must precede the reference through ss;
	   since the byte beyond the maximum may segfault */
	while ((maxlen > 0) && *ss) {
		ss++;
		maxlen--;
	}
	return ss - s;
}
