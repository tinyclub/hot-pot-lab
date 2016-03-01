#include <stdio.h>
#include <stdlib.h>


int lwip_file_open(const char *pathname, int flags, mode_t mode)
{
	return open(pathname, flags, mode);
}

int lwip_file_unlink(const char *a)
{
	return unlink(a);;
}

size_t lwip_file_write(int a, const void *b, size_t c)
{
	return write(a, b, c);
}

ssize_t lwip_file_read(int a, void *b, size_t c)
{
	return read(a, b, c);
}

