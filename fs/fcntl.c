
#include <dim-sum/mem.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/msdos_fs.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <linux/time.h>
#include <linux/utime.h>
#include <dim-sum/errno.h>

extern int fcntl_getlk(unsigned int, struct flock *);
extern int fcntl_setlk(unsigned int, unsigned int, struct flock *);
extern int sock_fcntl (struct file *, unsigned int cmd, unsigned long arg);

/**
 * xby_debug, 桩函数
 * 目前暂不实现文件锁
 */
int fcntl_getlk(unsigned int fd, struct flock *l)
{
	return 0;
}
int fcntl_setlk(unsigned int fd, unsigned int cmd, struct flock *l)
{
	return 0;
}


/**
 * xby_debug, 桩函数
 * 目前暂不实现socket的控制
 */
int sock_fcntl (struct file *f, unsigned int cmd, unsigned long arg)
{
	return 0;
}


/**
 * 复制文件句柄
 */
static int dupfd(unsigned int fd, unsigned int arg)
{
	struct files_struct *files = current->files;

	//参数检查
	if (fd >= NR_OPEN)
		return -EBADF;
	if (arg >= NR_OPEN)
		return -EINVAL;

	//获取文件锁
	spin_lock(&files->file_lock);
	//如果当前文件句柄不存在，则退出
	if (!files->fd_array[fd]) {
		spin_unlock(&files->file_lock);
		return -EBADF;
	}

	if ((arg != 0) //目标句柄被占用了
		&& files->fd_array[arg]) {
		spin_unlock(&files->file_lock);
		return -EMFILE;
	}
	//查找下一个可用句柄
	while (arg < NR_OPEN) {
		if (files->fd_array[arg])
			arg++;
		else
			break;
	}

	//无可用句柄了
	if (arg >= NR_OPEN) {
		spin_unlock(&files->file_lock);
		return -EMFILE;
	}
	//FD_CLR(arg, &current->files->close_on_exec);
	//复制文件描述符并增加引用计数
	files->fd_array[arg] = files->fd_array[fd];
	hold_file(files->fd_array[fd]);
	//释放锁
	spin_unlock(&files->file_lock);
	return arg;
}

/**
 * dup2系统调用
 */
asmlinkage int sys_dup2(unsigned int oldfd, unsigned int newfd)
{
	if (oldfd >= NR_OPEN)
		return -EBADF;
	if (newfd == oldfd)
		return newfd;

	if (newfd >= NR_OPEN)	
		return -EBADF;

	//首先关闭新句柄
	sys_close(newfd);
	//复制句柄
	return dupfd(oldfd,newfd);
}

/**
 * dup系统调用
 */
asmlinkage int sys_dup(unsigned int fildes)
{
	return dupfd(fildes,0);
}

/**
 * fcntl系统调用
 */
asmlinkage int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg)
{	
	struct file * filp;
	int ret = -EBADF;

	filp = fd2file(fd);
	if (!filp)
		return -EBADF;
	switch (cmd) {
		case F_DUPFD:
			ret = dupfd(fd,arg);
			break;
		case F_GETFD:
			ret = -EINVAL;
			break;
		case F_SETFD:
			if (arg&1)
				ret = -EINVAL;
			else
				ret = -EINVAL;
			break;
		case F_GETFL:
			ret = filp->f_flags;
			break;
		case F_SETFL://设置文件属性
			// 不能清除O_APPEND标志
			if (IS_APPEND(filp->f_inode) && !(arg & O_APPEND)) {
				ret = -EPERM;
				break;
			}

			//如果FASYNC标志发生了变化，则通知文件系统
			if ((arg & FASYNC) && !(filp->f_flags & FASYNC) &&
			    filp->f_op->fasync)
				filp->f_op->fasync(filp->f_inode, filp, 1);
			if (!(arg & FASYNC) && (filp->f_flags & FASYNC) &&
			    filp->f_op->fasync)
				filp->f_op->fasync(filp->f_inode, filp, 0);
			//只能设置O_APPEND | O_NONBLOCK | FASYNC这三个标志
			filp->f_flags &= ~(O_APPEND | O_NONBLOCK | FASYNC);
			filp->f_flags |= arg & (O_APPEND | O_NONBLOCK |
						FASYNC);
			ret = 0;
			break;
		case F_GETLK:
			ret = fcntl_getlk(fd, (struct flock *) arg);
			break;
		case F_SETLK:
			ret = fcntl_setlk(fd, cmd, (struct flock *) arg);
			break;
		case F_SETLKW:
			ret = fcntl_setlk(fd, cmd, (struct flock *) arg);
			break;
		case F_GETOWN:
			ret = filp->f_owner;
			break;
		case F_SETOWN:
			filp->f_owner = arg;
			if (S_ISSOCK (filp->f_inode->i_mode)) {
				ret = sock_fcntl (filp, F_SETOWN, arg);
				break;
			}
			ret = 0;
			break;
		default:
			/* sockets need a few special fcntls. */
			if (S_ISSOCK (filp->f_inode->i_mode)) {
			     ret = (sock_fcntl (filp, cmd, arg));
				 break;
			}
			ret = -EINVAL;
	}

	put_file(filp);

	return ret;
}

/**
 * kill与tty相关的所有任务
 */
void kill_fasync(struct fasync_struct *fa, int sig)
{
	while (fa) {//遍历所有任务
		if (fa->magic != FASYNC_MAGIC) {//安全性警告，防止内存问题
			printk("kill_fasync: bad magic number in "
			       "fasync_struct!\n");
			return;
		}
		//视情况杀死进程组或任务
		if (fa->fa_file->f_owner > 0)
			kill_proc(fa->fa_file->f_owner, sig, 1);
		else
			kill_pg(-fa->fa_file->f_owner, sig, 1);
		fa = fa->fa_next;
	}
}

