
#include <base/string.h>
#include <dim-sum/mem.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
//#include <linux/stat.h>
#include <fs/msdos_fs.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <linux/time.h>
#include <linux/utime.h>
#include <dim-sum/errno.h>

static void cp_new_stat(struct inode * inode, struct stat * statbuf)
{
	struct stat tmp;
	unsigned int blocks, indirect;

	memset(&tmp, 0, sizeof(tmp));
	tmp.st_dev = inode->i_dev;
	tmp.st_ino = inode->i_ino;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlink;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_rdev;
	tmp.st_size = inode->i_size;
#ifdef xby_debug
	if (inode->i_pipe)
		tmp.st_size = PIPE_SIZE(*inode);
#endif
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
/*
 * st_blocks and st_blksize are approximated with a simple algorithm if
 * they aren't supported directly by the filesystem. The minix and msdos
 * filesystems don't keep track of blocks, so they would either have to
 * be counted explicitly (by delving into the file itself), or by using
 * this simple algorithm to get a reasonable (although not 100% accurate)
 * value.
 */

/*
 * Use minix fs values for the number of direct and indirect blocks.  The
 * count is now exact for the minix fs except that it counts zero blocks.
 * Everything is in BLOCK_SIZE'd units until the assignment to
 * tmp.st_blksize.
 */
#define D_B   7
#define I_B   (BLOCK_SIZE / sizeof(unsigned short))

	if (!inode->i_blksize) {
		blocks = (tmp.st_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
		if (blocks > D_B) {
			indirect = (blocks - D_B + I_B - 1) / I_B;
			blocks += indirect;
			if (indirect > 1) {
				indirect = (indirect - 1 + I_B - 1) / I_B;
				blocks += indirect;
				if (indirect > 1)
					blocks++;
			}
		}
		tmp.st_blocks = (BLOCK_SIZE / 512) * blocks;
		tmp.st_blksize = BLOCK_SIZE;
	} else {
		tmp.st_blocks = inode->i_blocks;
		tmp.st_blksize = inode->i_blksize;
	}
	memcpy_tofs(statbuf,&tmp,sizeof(tmp));
}

asmlinkage int sys_stat(const char * filename, struct stat * statbuf)
{
	struct inode * inode;
	int error;

	error = verify_area(VERIFY_WRITE,statbuf,sizeof (*statbuf));
	if (error)
		return error;
	error = namei(filename,&inode);
	if (error)
		return error;
	cp_new_stat(inode,statbuf);
	iput(inode);
	return 0;
}


asmlinkage int sys_lstat(const char * filename, struct stat * statbuf)
{
	struct inode * inode;
	int error;

	error = verify_area(VERIFY_WRITE,statbuf,sizeof (*statbuf));
	if (error)
		return error;
	error = lnamei(filename,&inode);
	if (error)
		return error;
	cp_new_stat(inode,statbuf);
	iput(inode);
	return 0;
}


asmlinkage int sys_fstat(unsigned int fd, struct stat * statbuf)
{
	struct file * f;
	struct inode * inode;
	int error;

	error = verify_area(VERIFY_WRITE,statbuf,sizeof (*statbuf));
	if (error)
		return error;
	if (fd >= NR_OPEN || !(f=current->files->fd_array[fd]) || !(inode=f->f_inode))
		return -EBADF;
	cp_new_stat(inode,statbuf);
	return 0;
}

asmlinkage int sys_readlink(const char * path, char * buf, int bufsiz)
{
	struct inode * inode;
	int error;

	if (bufsiz <= 0)
		return -EINVAL;
	error = verify_area(VERIFY_WRITE,buf,bufsiz);
	if (error)
		return error;
	error = lnamei(path,&inode);
	if (error)
		return error;
	if (!inode->i_op || !inode->i_op->readlink) {
		iput(inode);
		return -EINVAL;
	}
	return inode->i_op->readlink(inode,buf,bufsiz);
}

