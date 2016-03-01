
#include <linux/types.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <asm/page.h>
#include <dim-sum/mem.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <fs/termios.h>
#include <fs/fcntl.h>
#include <dim-sum/sched.h>
#include <dim-sum/errno.h>

static int file_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
	int error;
	int block;

	switch (cmd) {
		case FIBMAP:
			if (filp->f_inode->i_op == NULL)
				return -EBADF;
		    	if (filp->f_inode->i_op->bmap == NULL)
				return -EINVAL;
			error = verify_area(VERIFY_WRITE,(void *) arg,4);
			if (error)
				return error;
			block = get_fs_long((long *) arg);
			block = filp->f_inode->i_op->bmap(filp->f_inode,block);
			put_fs_long(block,(long *) arg);
			return 0;
		case FIGETBSZ:
			if (filp->f_inode->i_sb == NULL)
				return -EBADF;
			error = verify_area(VERIFY_WRITE,(void *) arg,4);
			if (error)
				return error;
			put_fs_long(filp->f_inode->i_sb->s_blocksize,
			    (long *) arg);
			return 0;
		case FIONREAD:
			error = verify_area(VERIFY_WRITE,(void *) arg,4);
			if (error)
				return error;
			put_fs_long(filp->f_inode->i_size - filp->f_pos,
			    (long *) arg);
			return 0;
	}
	if (filp->f_op && filp->f_op->ioctl)
		return filp->f_op->ioctl(filp->f_inode, filp, cmd,arg);
	return -EINVAL;
}


asmlinkage int sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{	
	struct file * filp;
	int on;

	if (fd >= NR_OPEN || !(filp = current->files->fd_array[fd]))
		return -EBADF;
	switch (cmd) {
		case FIOCLEX:
#ifdef xby_debug
			FD_SET(fd, &current->files->close_on_exec);
#endif
			return 0;

		case FIONCLEX:
#ifdef xby_debug
			FD_CLR(fd, &current->files->close_on_exec);
#endif
			return 0;

		case FIONBIO:
			on = get_fs_long((unsigned long *) arg);
			if (on)
				filp->f_flags |= O_NONBLOCK;
			else
				filp->f_flags &= ~O_NONBLOCK;
			return 0;

		case FIOASYNC: /* O_SYNC is not yet implemented,
				  but it's here for completeness. */
			on = get_fs_long ((unsigned long *) arg);
			if (on)
				filp->f_flags |= O_SYNC;
			else
				filp->f_flags &= ~O_SYNC;
			return 0;

		default:
			if (filp->f_inode && S_ISREG(filp->f_inode->i_mode))
				return file_ioctl(filp,cmd,arg);

			if (filp->f_op && filp->f_op->ioctl)
				return filp->f_op->ioctl(filp->f_inode, filp, cmd,arg);

			return -EINVAL;
	}
}

