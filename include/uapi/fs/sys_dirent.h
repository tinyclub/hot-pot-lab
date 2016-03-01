/*
 * sys/dirent.h
 */

#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H


/* File types to use for d_type */
#define DT_UNKNOWN	 0
#define DT_FIFO		 1
#define DT_CHR		 2
#define DT_DIR		 4
#define DT_BLK		 6
#define DT_REG		 8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

extern int getdents(unsigned int, struct dirent *, unsigned int);

#endif				/* _SYS_DIRENT_H */
