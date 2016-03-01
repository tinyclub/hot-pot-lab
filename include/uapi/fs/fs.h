#ifndef _PHOENIX_FS_H
#define _PHOENIX_FS_H

/*
 * This file has definitions for some important file table
 * structures etc.
 */

#include <linux/linkage.h>
//#include <linux/limits.h>
#include <dim-sum/semaphore.h>
#include <dim-sum/wait.h>
#include <linux/types.h>
#include <fs/dirent.h>
#include <fs/vfs.h>
#include <dim-sum/syscall.h>
#include <dim-sum/errno.h>
#include <dim-sum/mem.h>

#define in_group_p(a) (1)

/*
 * It's silly to have NR_OPEN bigger than NR_FILE, but I'll fix
 * that later. Anyway, now the file code is no longer dependent
 * on bitmaps in unsigned longs, but uses the new fd_set structure..
 *
 * Some programs (notably those using select()) may have to be 
 * recompiled to take full advantage of the new limits..
 */
#undef NR_OPEN
#define NR_OPEN 256

#define NR_INODE 2048	/* this should be bigger than NR_FILE */
#define NR_FILE 1024	/* this can well be larger on a larger system */
#define NR_SUPER 32
#define NR_IHASH 131
#define BLOCK_SIZE 1024
#define BLOCK_SIZE_BITS 10

#define MAY_EXEC 1
#define MAY_WRITE 2
#define MAY_READ 4

#define READ 0
#define WRITE 1
#define READA 2		/* read-ahead - don't pause */
#define WRITEA 3	/* "write-ahead" - silly, but somewhat useful */

extern void buffer_init(void);
extern unsigned long inode_init(void);
extern unsigned long file_table_init(void);
extern unsigned long name_cache_init(void);

#define MAJOR(a) (int)((unsigned short)(a) >> 8)
#define MINOR(a) (int)((unsigned short)(a) & 0xFF)
#define MKDEV(a,b) ((int)((((a) & 0xff) << 8) | ((b) & 0xff)))
#define NODEV MKDEV(0,0)

#ifndef NULL
#define NULL ((void *) 0)
#endif

#define NIL_FILP	((struct file *)0)
#define SEL_IN		1
#define SEL_OUT		2
#define SEL_EX		4

/*
 * These are the fs-independent mount-flags: up to 16 flags are supported
 */
#define MS_RDONLY	 1 /* mount read-only */
#define MS_NOSUID	 2 /* ignore suid and sgid bits */
#define MS_NODEV	 4 /* disallow access to device special files */
#define MS_NOEXEC	 8 /* disallow program execution */
#define MS_SYNCHRONOUS	16 /* writes are synced at once */
#define MS_REMOUNT	32 /* alter flags of a mounted FS */
#define MS_SILENT	32768

//仅仅能添加，而不能修改已有内容
#define S_APPEND    256 /* append-only file */
#define S_IMMUTABLE 512 /* immutable file */

/*
 * Flags that can be altered by MS_REMOUNT
 */
#define MS_RMT_MASK (MS_RDONLY)

/*
 * Magic mount flag number. Has to be or-ed to the flag values.
 */
#define MS_MGC_VAL 0xC0ED0000 /* magic flag number to indicate "new" flags */
#define MS_MGC_MSK 0xffff0000 /* magic flag number mask */

/*
 * Note that read-only etc flags are inode-specific: setting some file-system
 * flags just means all the inodes inherit those flags by default. It might be
 * possible to override it selectively if you really wanted to with some
 * ioctl() that is not currently implemented.
 *
 * Exception: MS_RDONLY is always applied to the entire file system.
 */
#define IS_RDONLY(inode) (((inode)->i_sb) && ((inode)->i_sb->s_flags & MS_RDONLY))
#define IS_NOSUID(inode) ((inode)->i_flags & MS_NOSUID)
#define IS_NODEV(inode) ((inode)->i_flags & MS_NODEV)
#define IS_NOEXEC(inode) ((inode)->i_flags & MS_NOEXEC)
#define IS_SYNC(inode) ((inode)->i_flags & MS_SYNCHRONOUS)

#define IS_APPEND(inode) ((inode)->i_flags & S_APPEND)
#define IS_IMMUTABLE(inode) ((inode)->i_flags & S_IMMUTABLE)

/* the read-only stuff doesn't really belong here, but any other place is
   probably as bad and I don't want to create yet another include file. */

#define BLKROSET 4701 /* set device read-only (0 = read-write) */
#define BLKROGET 4702 /* get read-only status (0 = read_write) */
#define BLKRRPART 4703 /* re-read partition table */
#define BLKGETSIZE 4704 /* return device size */
#define BLKFLSBUF 4705 /* flush buffer cache */
#define BLKRASET 4706 /* Set read ahead for block device */
#define BLKRAGET 4707 /* get current read ahead setting */

/* These are a few other constants  only used by scsi  devices */

#define SCSI_IOCTL_GET_IDLUN 0x5382

/* Used to turn on and off tagged queuing for scsi devices */

#define SCSI_IOCTL_TAGGED_ENABLE 0x5383
#define SCSI_IOCTL_TAGGED_DISABLE 0x5384


#define BMAP_IOCTL 1	/* obsolete - kept for compatibility */
#define FIBMAP	   1	/* bmap access */
#define FIGETBSZ   2	/* get the block size used for bmap */

typedef char buffer_block[BLOCK_SIZE];

struct buffer_head {
	char * b_data;			/* pointer to data block (1024 bytes) */
	unsigned long b_size;		/* block size */
	unsigned long b_blocknr;	/* block number */
	dev_t b_dev;			/* device (0 = free) */
	unsigned short b_count;		/* users using this block */
	unsigned char b_uptodate;
	unsigned char b_dirt;		/* 0-clean,1-dirty */
	unsigned char b_lock;		/* 0 - ok, 1 -locked */
	unsigned char b_req;		/* 0 if the buffer has been invalidated */
	unsigned char b_list;		/* List that this buffer appears */
	unsigned char b_retain;         /* Expected number of times this will
					   be used.  Put on freelist when 0 */
	unsigned long b_flushtime;      /* Time when this (dirty) buffer should be written */
	unsigned long long b_lru_time;       /* Time when this buffer was last used. */
	wait_queue_head_t b_wait;
	struct buffer_head * b_prev;		/* doubly linked list of hash-queue */
	struct buffer_head * b_next;
	struct buffer_head * b_prev_free;	/* doubly linked list of buffers */
	struct buffer_head * b_next_free;
	struct buffer_head * b_this_page;	/* circular list of buffers in one page */
	struct buffer_head * b_reqnext;		/* request queue */
};


#include <fs/msdos_fs_i.h>
#include <fs/umsdos_fs_i.h>
#include <fs/ext2_fs_i.h>


#ifdef __KERNEL__

/**
 * 属性标志。用于notify_change，表示哪些属性发生了变化
 */
#define ATTR_MODE	1
#define ATTR_UID	2
#define ATTR_GID	4
#define ATTR_SIZE	8
#define ATTR_ATIME	16
#define ATTR_MTIME	32
#define ATTR_CTIME	64
#define ATTR_ATIME_SET	128
#define ATTR_MTIME_SET	256

/**
 * inode属性描述符，用于notify_change
 * 用于通知notify_change哪些属性发生了变化
 */
struct iattr {
	//发生变化的属性,如ATTR_SIZE
	unsigned int	ia_valid;
	umode_t		ia_mode;
	uid_t		ia_uid;
	gid_t		ia_gid;
	//变化后的长度
	off_t		ia_size;
	time_t		ia_atime;
	time_t		ia_mtime;
	time_t		ia_ctime;
};

struct inode {
	dev_t		i_dev;
	unsigned long	i_ino;
	umode_t		i_mode;
	nlink_t		i_nlink;
	uid_t		i_uid;
	gid_t		i_gid;
	dev_t		i_rdev;
	off_t		i_size;
	time_t		i_atime;
	time_t		i_mtime;
	time_t		i_ctime;
	unsigned long	i_blksize;
	unsigned long	i_blocks;
	unsigned long	i_version;
	semaphore i_sem;
	struct inode_operations * i_op;
	//节点所在的超级块
	struct super_block * i_sb;
	wait_queue_head_t i_wait;
	struct file_lock * i_flock;
	struct vm_area_struct * i_mmap;
	struct inode * i_next, * i_prev;
	struct inode * i_hash_next, * i_hash_prev;
	struct inode * i_bound_to, * i_bound_by;
	struct inode * i_mount;
	/**
	 * 引用计数器。
	 */
	atomic_t		i_count;
	unsigned short i_wcount;
	unsigned short i_flags;
	unsigned char i_lock;
	unsigned char i_dirt;
	unsigned char i_pipe;
	unsigned char i_sock;
	unsigned char i_seek;
	unsigned char i_update;
	union {
		struct msdos_inode_info msdos_i;
		struct ext2_inode_info ext2_i;
		struct umsdos_inode_info umsdos_i;
		void * generic_ip;
	} u;
};

struct file {
	//访问权限，如O_RDWR
	mode_t f_mode;
	//当前读写位置
	loff_t f_pos;
	//访问标志，如O_APPEND
	unsigned short f_flags;
	/**
	 * 文件对象的引用计数。
	 * 指引用文件对象的进程数。内核也可能增加此计数。
	 */
	atomic_t		f_count;
	//预读位置
	off_t f_reada;
	struct file *f_next, *f_prev;
	/**
	 * 接收SIGIO的进程ID
	 * 如果是进程组，则为负数
	 */
	int f_owner;		/* pid or -pgrp where SIGIO should be sent */
	/* 文件对应的inode */
	struct inode * f_inode;
	//文件操作回调
	struct file_operations * f_op;
	unsigned long f_version;
	void *private_data;	/* needed for tty driver, and maybe others */
};

extern void release_file(struct file *file);

//无效的文件描述符
#define INVALIDATE_FILE ((struct file *)1)
//未使用的文件描述符
#define UNUSED_FILE (NULL)

struct file_lock {
	struct file_lock *fl_next;	/* singly linked list for this inode  */
	struct file_lock *fl_nextlink;	/* doubly linked list of all locks */
	struct file_lock *fl_prevlink;	/* used to simplify lock removal */
	struct task_struct *fl_owner;
	struct wait_queue *fl_wait;
	char fl_type;
	char fl_whence;
	off_t fl_start;
	off_t fl_end;
};

/**
 * 异步等待文件描述符
 */
struct fasync_struct {
	int    magic;
	struct fasync_struct	*fa_next; /* singly linked list */
	struct file 		*fa_file;
};

#define FASYNC_MAGIC 0x4601

#include <fs/msdos_fs_sb.h>
#include <fs/ext2_fs_sb.h>


struct super_block {
	dev_t s_dev;
	unsigned long s_blocksize;
	unsigned char s_blocksize_bits;
	unsigned char s_lock;
	unsigned char s_rd_only;
	unsigned char s_dirt;
	struct file_system_type *s_type;
	struct super_operations *s_op;
	unsigned long s_flags;
	unsigned long s_magic;
	unsigned long s_time;
	//本文件系统绑定的目录节点
	struct inode * s_covered;
	//文件系统被mount的根节点
	struct inode * s_mounted;
	wait_queue_head_t s_wait;
	union {
		struct msdos_sb_info msdos_sb;
		struct ext2_sb_info ext2_sb;
		void *generic_sbp;
	} u;
};

struct file_operations {
	//设置读写位置
	int (*lseek) (struct inode *, struct file *, off_t, int);
	//读取节点的数据到缓冲区中
	int (*read) (struct inode *, struct file *, char *, int);
	int (*write) (struct inode *, struct file *, const char *, int);
	//读取目录中的文件名列表
	int (*readdir) (struct inode *, struct file *, struct dirent *, int);
	int (*select) (struct inode *, struct file *, int, wait_queue_t *);
	int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long);
	int (*mmap) (struct inode *, struct file *, struct vm_area_struct *);
	//打开文件时，文件系统的回调函数。由各文件系统定义。
	int (*open) (struct inode *, struct file *);
	//当最终关闭节点时，文件系统进行一些清理工作
	void (*release) (struct inode *, struct file *);
	int (*fsync) (struct inode *, struct file *);
	/**
	 * 这个操作用来通知设备其FASYNC标志发生了变化。
	 * 如果设备不支持异步通知，该字段可以是NULL。
	 */
	int (*fasync) (struct inode *, struct file *, int);
	int (*check_media_change) (dev_t dev);
	int (*revalidate) (dev_t dev);
};

struct inode_operations {
	struct file_operations * default_file_ops;
	int (*create) (struct inode *,const char *,int,int,struct inode **);
	/**
	 * 在索引节点中，搜索目录中的文件。
	 */
	int (*lookup) (struct inode *,const char *,int,struct inode **);
	int (*link) (struct inode *,struct inode *,const char *,int);
	int (*unlink) (struct inode *,const char *,int);
	int (*symlink) (struct inode *,const char *,int,const char *);
	int (*mkdir) (struct inode *,const char *,int,int);
	int (*rmdir) (struct inode *,const char *,int);
	int (*mknod) (struct inode *,const char *,int,int,int);
	int (*rename) (struct inode *,const char *,int,struct inode *,const char *,int);
	int (*readlink) (struct inode *,char *,int);
	int (*follow_link) (struct inode *,struct inode *,int,int,struct inode **);
	int (*bmap) (struct inode *,int);
	void (*truncate) (struct inode *);
	int (*permission) (struct inode *, int);
	int (*smap) (struct inode *,int);
};

struct super_operations {
	void (*read_inode) (struct inode *);
	int (*notify_change) (struct inode *, struct iattr *);
	void (*write_inode) (struct inode *);
	void (*put_inode) (struct inode *);
	void (*put_super) (struct super_block *);
	void (*write_super) (struct super_block *);
	void (*statfs) (struct super_block *, struct statfs *);
	int (*remount_fs) (struct super_block *, int *, char *);
};

struct file_system_type {
	struct super_block *(*read_super) (struct super_block *, void *, int);
	char *name;
	int requires_dev;
	struct file_system_type * next;
};


extern int register_filesystem(struct file_system_type *);
extern int unregister_filesystem(struct file_system_type *);



extern void kill_fasync(struct fasync_struct *fa, int sig);

extern int getname(const char * filename, char **result);
extern void putname(char * name);

int register_chrdev(unsigned int major, const char * name, struct file_operations *fops);
	int unregister_chrdev(unsigned int major, const char * name);

extern int register_blkdev(unsigned int, const char *, struct file_operations *);
extern int unregister_blkdev(unsigned int major, const char * name);
extern int blkdev_open(struct inode * inode, struct file * filp);
extern struct file_operations def_blk_fops;
extern struct inode_operations blkdev_inode_operations;

extern int register_chrdev(unsigned int, const char *, struct file_operations *);
extern int unregister_chrdev(unsigned int major, const char * name);
extern int chrdev_open(struct inode * inode, struct file * filp);
extern struct file_operations def_chr_fops;
extern struct inode_operations chrdev_inode_operations;

#ifdef xby_debug
extern void init_fifo(struct inode * inode);
#else
static inline void init_fifo(struct inode * inode)
{
}
#endif

extern struct file_operations connecting_fifo_fops;
extern struct file_operations read_fifo_fops;
extern struct file_operations write_fifo_fops;
extern struct file_operations rdwr_fifo_fops;
extern struct file_operations read_pipe_fops;
extern struct file_operations write_pipe_fops;
extern struct file_operations rdwr_pipe_fops;

extern struct file_system_type *get_fs_type(char *name);

extern int fs_may_mount(dev_t dev);
extern int fs_may_umount(dev_t dev, struct inode * mount_root);
extern int fs_may_remount_ro(dev_t dev);

extern struct file *first_file;
extern int nr_files;
extern struct super_block super_blocks[NR_SUPER];

extern int shrink_buffers(unsigned int priority);
extern void refile_buffer(struct buffer_head * buf);
extern void set_writetime(struct buffer_head * buf, int flag);
extern void refill_freelist(int size);

extern struct buffer_head ** buffer_pages;
extern int nr_buffers;
extern int buffermem;
extern int nr_buffer_heads;

#define BUF_CLEAN 0
#define BUF_UNSHARED 1 /* Buffers that were shared but are not any more */
#define BUF_LOCKED 2   /* Buffers scheduled for write */
#define BUF_LOCKED1 3  /* Supers, inodes */
#define BUF_DIRTY 4    /* Dirty buffers, not yet scheduled for write */
#define BUF_SHARED 5   /* Buffers shared */
#define NR_LIST 6

extern inline void mark_buffer_clean(struct buffer_head * bh)
{
  if(bh->b_dirt) {
    bh->b_dirt = 0;
    if(bh->b_list == BUF_DIRTY) refile_buffer(bh);
  }
}

extern inline void mark_buffer_dirty(struct buffer_head * bh, int flag)
{
  if(!bh->b_dirt) {
    bh->b_dirt = 1;
    set_writetime(bh, flag);
    if(bh->b_list != BUF_DIRTY) refile_buffer(bh);
  }
}


extern int check_disk_change(dev_t dev);
extern void invalidate_inodes(dev_t dev);
extern void invalidate_buffers(dev_t dev);
extern int floppy_is_wp(int minor);
extern void sync_inodes(dev_t dev);
extern void sync_dev(dev_t dev);
extern int fsync_dev(dev_t dev);
extern void sync_supers(dev_t dev);
extern int bmap(struct inode * inode,int block);
extern int notify_change(struct inode *, struct iattr *);
extern int namei(const char * pathname, struct inode ** res_inode);
extern int lnamei(const char * pathname, struct inode ** res_inode);
extern int permission(struct inode * inode,int mask);
extern int get_write_access(struct inode * inode);
extern void put_write_access(struct inode * inode);
extern int open_namei(const char * pathname, int flag, int mode,
	struct inode ** res_inode, struct inode * base);
extern int do_mknod(const char * filename, int mode, dev_t dev);
extern void iput(struct inode * inode);
extern struct inode * __iget(struct super_block * sb,int nr,int crsmnt);
extern struct inode * get_empty_inode(void);
extern void insert_inode_hash(struct inode *);
extern void clear_inode(struct inode *);
extern struct inode * get_pipe_inode(void);
extern struct file * get_empty_filp(void);
extern struct buffer_head * get_hash_table(dev_t dev, int block, int size);
extern struct buffer_head * getblk(dev_t dev, int block, int size);
extern void ll_rw_block(int rw, int nr, struct buffer_head * bh[]);
extern void ll_rw_page(int rw, int dev, int nr, char * buffer);
extern void ll_rw_swap_file(int rw, int dev, unsigned int *b, int nb, char *buffer);
extern int is_read_only(int dev);
extern void brelse(struct buffer_head * buf);
extern void set_blocksize(dev_t dev, int size);
extern struct buffer_head * bread(dev_t dev, int block, int size);
extern unsigned long bread_page(unsigned long addr,dev_t dev,int b[],int size,int no_share);
extern struct buffer_head * breada(dev_t dev,int block, int size, 
				   unsigned int pos, unsigned int filesize);
extern void put_super(dev_t dev);
unsigned long generate_cluster(dev_t dev, int b[], int size);
extern dev_t ROOT_DEV;

extern void show_buffers(void);
extern void mount_root(void);

extern int char_read(struct inode *, struct file *, char *, int);
extern int block_read(struct inode *, struct file *, char *, int);
extern int read_ahead[];

extern int char_write(struct inode *, struct file *, char *, int);
extern int block_write(struct inode *, struct file *,const char *, int);

extern int generic_mmap(struct inode *, struct file *, struct vm_area_struct *);

extern int block_fsync(struct inode *, struct file *);
extern int file_fsync(struct inode *, struct file *);

extern void dcache_add(struct inode *, const char *, int, unsigned long);
extern int dcache_lookup(struct inode *, const char *, int, unsigned long *);

extern int inode_change_ok(struct inode *, struct iattr *);
extern void inode_setattr(struct inode *, struct iattr *);

extern inline struct inode * iget(struct super_block * sb,int nr)
{
	return __iget(sb,nr,1);
}

#define memcpy_fromfs(to, from, n) memcpy((to),(from),(n))

#define memcpy_tofs(to, from, n) memcpy((to),(from),(n))

#define put_fs_byte(x,addr) (*(char *)(addr) = (x))
#define put_fs_long(x,addr) (*(long *)(addr) = (x))
#define put_fs_word(x,addr) (*(short *)(addr) = (x))

#define get_fs_byte(addr) get_user_byte((char *)(addr))
static inline unsigned char get_user_byte(const char *addr)
{
	return *addr;
}
#define get_fs_long(addr) get_user_long((long *)(addr))
static inline unsigned long get_user_long(const long *addr)
{
	return *addr;
}
static inline unsigned short get_user_word(const short *addr)
{
	return *addr;
}
extern void kill_fasync(struct fasync_struct *fa, int sig);

#define get_fs_word(addr) get_user_word((short *)(addr))



//xby_debug
#define GFP_BUFFER 0
#define GFP_NOBUFFER 0
#define suser() (1)
#define fsuser() (1)


#endif /* __KERNEL__ */


#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
/**
 * 设置用户和组的执行时ID
 */
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#ifdef __KERNEL__
#define S_IRWXUGO	(S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO	(S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO		(S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO		(S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO		(S_IXUSR|S_IXGRP|S_IXOTH)
#endif




#include <kapi/dim-sum/time.h>
#define FS_CURRENT_TIME get_jiffies_64()
extern unsigned long event;
/*
 * Those macros may have been defined in <gnu/types.h>. But we always
 * use the ones here. 
 */
#undef __FDSET_LONGS
#define __FDSET_LONGS 8

#define hold_file(x)	atomic_inc(&(x)->f_count)
#define put_file(x)	atomic_dec(&(x)->f_count)

extern struct file *fd2file(unsigned int fd);

#define kfree_s(obj, size) dim_sum_mem_free(obj)
extern int verify_area(int type, const void * addr, unsigned long size);

#endif

