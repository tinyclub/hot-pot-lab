#include <base/string.h>
#include <linux/time.h>
#include <fs/fs.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <fs/proc_fs.h>
#include <linux/time.h>
#include <asm/errno.h>
#include <dim-sum/mem.h>


/* forward references */
static int proc_readnet(struct inode * inode, struct file * file,
			 char * buf, int count);
static int proc_readnetdir(struct inode *, struct file *,
			   struct dirent *, int);
static int proc_lookupnet(struct inode *,const char *,int,struct inode **);

/* the get_*_info() functions are in the net code, and are configured
   in via the standard mechanism... */
extern int unix_get_info(char *, char **, off_t, int);
#ifdef CONFIG_INET
extern int tcp_get_info(char *, char **, off_t, int);
extern int udp_get_info(char *, char **, off_t, int);
extern int raw_get_info(char *, char **, off_t, int);
extern int arp_get_info(char *, char **, off_t, int);
extern int rarp_get_info(char *, char **, off_t, int);
extern int dev_get_info(char *, char **, off_t, int);
extern int rt_get_info(char *, char **, off_t, int);
extern int snmp_get_info(char *, char **, off_t, int);
extern int afinet_get_info(char *, char **, off_t, int);
#if	defined(CONFIG_WAVELAN)
extern int wavelan_get_info(char *, char **, off_t, int);
#endif	/* defined(CONFIG_WAVELAN) */
#ifdef CONFIG_IP_ACCT
extern int ip_acct_procinfo(char *, char **, off_t, int, int);
#endif /* CONFIG_IP_ACCT */
#ifdef CONFIG_IP_FIREWALL
extern int ip_fw_blk_procinfo(char *, char **, off_t, int, int);
extern int ip_fw_fwd_procinfo(char *, char **, off_t, int, int);
#endif /* CONFIG_IP_FIREWALL */
extern int ip_msqhst_procinfo(char *, char **, off_t, int);
extern int ip_mc_procinfo(char *, char **, off_t, int);
#endif /* CONFIG_INET */
#ifdef CONFIG_IPX
extern int ipx_get_info(char *, char **, off_t, int);
extern int ipx_rt_get_info(char *, char **, off_t, int);
extern int ipx_get_interface_info(char *, char **, off_t , int);
#endif /* CONFIG_IPX */
#ifdef CONFIG_AX25
extern int ax25_get_info(char *, char **, off_t, int);
extern int ax25_rt_get_info(char *, char **, off_t, int);
#ifdef CONFIG_NETROM
extern int nr_get_info(char *, char **, off_t, int);
extern int nr_nodes_get_info(char *, char **, off_t, int);
extern int nr_neigh_get_info(char *, char **, off_t, int);
#endif /* CONFIG_NETROM */
#endif /* CONFIG_AX25 */
#ifdef CONFIG_ATALK
extern int atalk_get_info(char *, char **, off_t, int);
extern int atalk_rt_get_info(char *, char **, off_t, int);
extern int atalk_if_get_info(char *, char **, off_t, int);
#endif

/**
 * 这是演示示例，请根据情况修改
 */
int unix_get_info(char *buffer, char **start, off_t offset, int length)
{
	int i = 0;
  	off_t begin=0;
  	int len=0;
	
  	len += sprintf(buffer, "Num RefCount Protocol Flags    Type St Path\n");
	len += sprintf(buffer+len, "%2d: %08X %08X %08X %04X %02X", i,
					1,
					2,
					0,
					0,
					0
				);
	
	*start=buffer+(offset-begin);
	len-=(offset-begin);
	if(len>length)
		len=length;
	return len;
}

static struct file_operations proc_net_operations = {
	NULL,			/* lseek - default */
	proc_readnet,		/* read - bad */
	NULL,			/* write - bad */
	proc_readnetdir,	/* readdir */
	NULL,			/* select - default */
	NULL,			/* ioctl - default */
	NULL,			/* mmap */
	NULL,			/* no special open code */
	NULL,			/* no special release code */
	NULL			/* can't fsync */
};

/*
 * proc directories can do almost nothing..
 */
struct inode_operations proc_net_inode_operations = {
	&proc_net_operations,	/* default net directory file-ops */
	NULL,			/* create */
	proc_lookupnet,		/* lookup */
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

static struct proc_dir_entry net_dir[] = {
	{ PROC_NET,		1, "." },
	{ PROC_ROOT_INO,	2, ".." },
	{ PROC_NET_UNIX,	4, "unix" },
	{ 0, 0, NULL }
};

#define NR_NET_DIRENTRY ((sizeof (net_dir))/(sizeof (net_dir[0])) - 1)

static int proc_lookupnet(struct inode * dir,const char * name, int len,
	struct inode ** result)
{
	struct proc_dir_entry *de;

	*result = NULL;
	if (!dir)
		return -ENOENT;
	if (!S_ISDIR(dir->i_mode)) {
		iput(dir);
		return -ENOENT;
	}
	for (de = net_dir ; de->name ; de++) {
		if (!proc_match(len, name, de))
			continue;
		*result = iget(dir->i_sb, de->low_ino);
		iput(dir);
		if (!*result)
			return -ENOENT;
		return 0;
	}
	iput(dir);
	return -ENOENT;
}

static int proc_readnetdir(struct inode * inode, struct file * filp,
	struct dirent * dirent, int count)
{
	struct proc_dir_entry * de;
	unsigned int ino;
	int i,j;

	if (!inode || !S_ISDIR(inode->i_mode))
		return -EBADF;
	ino = inode->i_ino;
	if (((unsigned) filp->f_pos) < NR_NET_DIRENTRY) {
		de = net_dir + filp->f_pos;
		filp->f_pos++;
		i = de->namelen;
		ino = de->low_ino;
		put_fs_long(ino, &dirent->d_ino);
		put_fs_word(i,&dirent->d_reclen);
		put_fs_byte(0,i+dirent->d_name);
		j = i;
		while (i--)
			put_fs_byte(de->name[i], i+dirent->d_name);
		return j;
	}
	return 0;
}


#define PROC_BLOCK_SIZE	(3*1024)		/* 4K page size but our output routines use some slack for overruns */

static int proc_readnet(struct inode * inode, struct file * file,
			char * buf, int count)
{
	char * page;
	int length;
	unsigned int ino;
	int bytes=count;
	int thistime;
	int copied=0;
	char *start;

	if (count < 0)
		return -EINVAL;
	if (!(page = (char*) dim_sum_pages_alloc(0, MEM_NORMAL)))
		return -ENOMEM;
	ino = inode->i_ino;

	while(bytes>0)
	{
		thistime=bytes;
		if(bytes>PROC_BLOCK_SIZE)
			thistime=PROC_BLOCK_SIZE;

		switch (ino) 
		{
			case PROC_NET_UNIX:
				length = unix_get_info(page,&start,file->f_pos,thistime);
				break;

			default:
				dim_sum_pages_free((void *) page);
				return -EBADF;
		}
		
		/*
 		 *	We have been given a non page aligned block of
		 *	the data we asked for + a bit. We have been given
 		 *	the start pointer and we know the length.. 
		 */

		if (length <= 0)
			break;
		/*
 		 *	Copy the bytes
		 */
		memcpy_tofs(buf+copied, start, length);
		file->f_pos+=length;	/* Move down the file */
		bytes-=length;
		copied+=length;
		if(length<thistime)
			break;	/* End of file */
	}
	dim_sum_pages_free((void *) page);
	return copied;

}

