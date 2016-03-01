#include <fs/fs.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <fs/proc_fs.h>
#include <linux/errno.h>

static int proc_readroot(struct inode *, struct file *, struct dirent *, int);
static int proc_lookuproot(struct inode *,const char *,int,struct inode **);


static struct proc_dir_entry root_dir[] = {
	{ PROC_ROOT_INO,	1, "." },
	{ PROC_ROOT_INO,	2, ".." },
	{ PROC_LOADAVG,		7, "loadavg" },
	{ PROC_UPTIME,		6, "uptime" },
	{ PROC_MEMINFO,		7, "meminfo" },
	{ PROC_NET,		3, "net" },
	{ PROC_VERSION,		7, "version" },
	{ PROC_CPUINFO,		7, "cpuinfo" },
	{ PROC_INTERRUPTS,	10,"interrupts" },
};

#define NR_ROOT_DIRENTRY ((sizeof (root_dir))/(sizeof (root_dir[0])))

static int proc_readroot(struct inode * inode, struct file * filp,
	struct dirent * dirent, int count)
{
	unsigned int nr;
	int i,j;

	if (!inode || !S_ISDIR(inode->i_mode))
		return -EBADF;
//repeat:
	nr = filp->f_pos;
	if (nr < NR_ROOT_DIRENTRY) {
		struct proc_dir_entry * de = root_dir + nr;

		filp->f_pos++;
		i = de->namelen;
		put_fs_long(de->low_ino, &dirent->d_ino);
		put_fs_word(i,&dirent->d_reclen);
		put_fs_byte(0,i+dirent->d_name);
		j = i;
		while (i--)
			put_fs_byte(de->name[i], i+dirent->d_name);
		return j;
	}

	return 0;
}

static struct file_operations proc_root_operations = {
	NULL,			/* lseek - default */
	NULL,			/* read - bad */
	NULL,			/* write - bad */
	proc_readroot,		/* readdir */
	NULL,			/* select - default */
	NULL,			/* ioctl - default */
	NULL,			/* mmap */
	NULL,			/* no special open code */
	NULL,			/* no special release code */
	NULL			/* no fsync */
};


static int proc_lookuproot(struct inode * dir,const char * name, int len,
	struct inode ** result)
{
	int i, ino;

	*result = NULL;
	if (!dir)
		return -ENOENT;
	if (!S_ISDIR(dir->i_mode)) {
		iput(dir);
		return -ENOENT;
	}
	i = NR_ROOT_DIRENTRY;
	while (i-- > 0 && !proc_match(len,name,root_dir+i))
		/* nothing */;
	if (i >= 0) {
		ino = root_dir[i].low_ino;
		if (ino == PROC_ROOT_INO) {
			*result = dir;
			return 0;
		}
	} else {
		return -ENOENT;
	}
	if (!(*result = iget(dir->i_sb,ino))) {
		iput(dir);
		return -ENOENT;
	}
	iput(dir);
	return 0;
}


/*
 * proc directories can do almost nothing..
 */
struct inode_operations proc_root_inode_operations = {
	&proc_root_operations,	/* default base directory file-ops */
	NULL,			/* create */
	proc_lookuproot,	/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* bmap */
	NULL,			/* truncate */
	NULL			/* permission */
};

