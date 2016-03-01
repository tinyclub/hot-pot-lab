#include <base/string.h>
#include <linux/time.h>
#include <fs/fs.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <fs/proc_fs.h>
#include <linux/time.h>
#include <dim-sum/timer.h>
#include <fs/locks.h>


void proc_read_inode(struct inode * inode)
{
	unsigned long ino, pid;
	
	inode->i_op = NULL;
	inode->i_mode = 0;
	inode->i_uid = 0;
	inode->i_gid = 0;
	inode->i_nlink = 1;
	inode->i_size = 0;
	inode->i_mtime = inode->i_atime = inode->i_ctime = FS_CURRENT_TIME;
	inode->i_blocks = 0;
	inode->i_blksize = 1024;
	ino = inode->i_ino;
	pid = ino >> 16;
	
	if (ino == PROC_ROOT_INO) {
		inode->i_mode = S_IFDIR | S_IRUGO | S_IXUGO;
		inode->i_nlink = 2;
		inode->i_op = &proc_root_inode_operations;
		return;
	}

	if (!pid) {
		switch (ino) {
			case PROC_NET:
				inode->i_mode = S_IFDIR | S_IRUGO | S_IXUGO;
				inode->i_nlink = 2;
				inode->i_op = &proc_net_inode_operations;
				break;
			default:
				inode->i_mode = S_IFREG | S_IRUGO;
				inode->i_op = &proc_array_inode_operations;
				break;
		}
		return;
	}
	return;
}


static struct super_operations proc_sops = { 
	proc_read_inode,
	NULL,
	proc_write_inode,
	proc_put_inode,
	proc_put_super,
	NULL,
	proc_statfs,
	NULL
};

static int parse_options(char *options,uid_t *uid,gid_t *gid)
{
	char *this_char,*value;

	*uid = current->uid;
	*gid = current->gid;
	if (!options) return 1;
	for (this_char = strtok(options,","); this_char; this_char = strtok(NULL,",")) {
		if ((value = strchr(this_char,'=')) != NULL)
			*value++ = 0;
		if (!strcmp(this_char,"uid")) {
			if (!value || !*value)
				return 0;
			*uid = simple_strtoul(value,&value,0);
			if (*value)
				return 0;
		}
		else if (!strcmp(this_char,"gid")) {
			if (!value || !*value)
				return 0;
			*gid = simple_strtoul(value,&value,0);
			if (*value)
				return 0;
		}
		else return 0;
	}
	return 1;
}

struct super_block *proc_read_super(struct super_block *s,void *data, 
				    int silent)
{
	lock_super(s);
	s->s_blocksize = 1024;
	s->s_blocksize_bits = 10;
	s->s_magic = PROC_SUPER_MAGIC;
	s->s_op = &proc_sops;
	unlock_super(s);
	if (!(s->s_mounted = iget(s,PROC_ROOT_INO))) {
		s->s_dev = 0;
		printk("get root inode failed\n");
		return NULL;
	}
	parse_options(data, &s->s_mounted->i_uid, &s->s_mounted->i_gid);
	return s;
}

void proc_statfs(struct super_block *sb, struct statfs *buf)
{
	put_fs_long(PROC_SUPER_MAGIC, &buf->f_type);
	put_fs_long(PAGE_SIZE/sizeof(long), &buf->f_bsize);
	put_fs_long(0, &buf->f_blocks);
	put_fs_long(0, &buf->f_bfree);
	put_fs_long(0, &buf->f_bavail);
	put_fs_long(0, &buf->f_files);
	put_fs_long(0, &buf->f_ffree);
	put_fs_long(NAME_MAX, &buf->f_namelen);
	/* Don't know what value to put in buf->f_fsid */
}


void proc_put_inode(struct inode *inode)
{
	if (inode->i_nlink)
		return;
	inode->i_size = 0;
}

void proc_write_inode(struct inode * inode)
{
	inode->i_dirt=0;
}

void proc_put_super(struct super_block *sb)
{
	lock_super(sb);
	sb->s_dev = 0;
	unlock_super(sb);
}


