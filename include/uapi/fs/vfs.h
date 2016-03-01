#ifndef _LINUX_VFS_H
#define _LINUX_VFS_H
/* The kernel calls this struct dirent64 */
struct dirent {
	uint64_t	d_ino;
	int64_t		d_off;
	unsigned short	d_reclen;
	unsigned char	d_type;
	char		d_name[256];
};


#endif

