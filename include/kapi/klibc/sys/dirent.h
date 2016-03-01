/*
 * sys/dirent.h
 */

#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#include <stdint.h>

/* The kernel calls this struct dirent64 */
#include <fs/vfs.h>
#include <fs/sys_dirent.h>

__extern int getdents(unsigned int, struct dirent *, unsigned int);

#endif				/* _SYS_DIRENT_H */
