#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>

extern int printk(const char *fmt, ...);
int creat_main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printk("usage:creat [file]\n");
		return 0;
	}
	
	return creat(argv[1], 0);
}

