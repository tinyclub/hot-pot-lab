
#include <dim-sum/mem.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/msdos_fs.h>
#include <dim-sum/sched.h>
#include <dim-sum/syscall.h>
#include <asm/current.h>
#include <linux/utime.h>
#include <dim-sum/timer.h>
#include <dim-sum/errno.h>

extern void fcntl_remove_locks(struct task_struct *, struct file *);
void fcntl_remove_locks(struct task_struct *task, struct file *filp)
{
}

asmlinkage int sys_ustat(int dev, struct ustat * ubuf)
{
	return -ENOSYS;
}

asmlinkage int sys_statfs(const char * path, struct statfs * buf)
{
	struct inode * inode;
	int error;

	error = verify_area(VERIFY_WRITE, buf, sizeof(struct statfs));
	if (error)
		return error;
	error = namei(path,&inode);
	if (error)
		return error;
	if (!inode->i_sb->s_op->statfs) {
		iput(inode);
		return -ENOSYS;
	}
	inode->i_sb->s_op->statfs(inode->i_sb, buf);
	iput(inode);
	return 0;
}

asmlinkage int sys_fstatfs(unsigned int fd, struct statfs * buf)
{
	struct inode * inode;
	struct file * file;
	int error;

	error = verify_area(VERIFY_WRITE, buf, sizeof(struct statfs));
	if (error)
		return error;
	if (fd >= NR_OPEN || !(file = current->files->fd_array[fd]))
		return -EBADF;
	if (!(inode = file->f_inode))
		return -ENOENT;
	if (!inode->i_sb->s_op->statfs)
		return -ENOSYS;
	inode->i_sb->s_op->statfs(inode->i_sb, buf);
	return 0;
}

asmlinkage int sys_truncate(const char * path, unsigned int length)
{
	struct inode * inode;
	int error;
	struct iattr newattrs;

	error = namei(path,&inode);
	if (error)
		return error;
	if (S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -EACCES;
	}
	if (IS_RDONLY(inode)) {
		iput(inode);
		return -EROFS;
	}
	if (IS_IMMUTABLE(inode) || IS_APPEND(inode)) {
		iput(inode);
		return -EPERM;
	}
	error = get_write_access(inode);
	if (error) {
		iput(inode);
		return error;
	}
	inode->i_size = newattrs.ia_size = length;
	if (inode->i_op && inode->i_op->truncate)
		inode->i_op->truncate(inode);
	newattrs.ia_ctime = newattrs.ia_mtime = FS_CURRENT_TIME;
	newattrs.ia_valid = ATTR_SIZE | ATTR_CTIME | ATTR_MTIME;
	inode->i_dirt = 1;
	error = notify_change(inode, &newattrs);
	put_write_access(inode);
	iput(inode);
	return error;
}

asmlinkage int sys_ftruncate(unsigned int fd, unsigned int length)
{
	struct inode * inode;
	struct file * file;
	struct iattr newattrs;

	if (fd >= NR_OPEN || !(file = current->files->fd_array[fd]))
		return -EBADF;
	if (!(inode = file->f_inode))
		return -ENOENT;
	if (S_ISDIR(inode->i_mode) || !(file->f_mode & 2))
		return -EACCES;
	if (IS_IMMUTABLE(inode) || IS_APPEND(inode))
		return -EPERM;
	inode->i_size = newattrs.ia_size = length;
	if (inode->i_op && inode->i_op->truncate)
		inode->i_op->truncate(inode);
	newattrs.ia_ctime = newattrs.ia_mtime = FS_CURRENT_TIME;
	newattrs.ia_valid = ATTR_SIZE | ATTR_CTIME | ATTR_MTIME;
	inode->i_dirt = 1;
	return notify_change(inode, &newattrs);
}

/* If times==NULL, set access and modification to current time,
 * must be owner or have write permission.
 * Else, update from *times, must be owner or super user.
 */
asmlinkage int sys_utime(const char * filename, const struct utimbuf * times)
{
	struct inode * inode;
	long actime,modtime;
	int error;
	unsigned int flags = 0;
	struct iattr newattrs;

	error = namei(filename,&inode);
	if (error)
		return error;
	if (IS_RDONLY(inode)) {
		iput(inode);
		return -EROFS;
	}
	/* Don't worry, the checks are done in inode_change_ok() */
	if (times) {
		error = verify_area(VERIFY_READ, times, sizeof(*times));
		if (error) {
			iput(inode);
			return error;
		}
		actime = get_fs_long((unsigned long *) &times->actime);
		modtime = get_fs_long((unsigned long *) &times->modtime);
		newattrs.ia_ctime = FS_CURRENT_TIME;
		flags = ATTR_ATIME_SET | ATTR_MTIME_SET;
	} else {
		actime = modtime = newattrs.ia_ctime = FS_CURRENT_TIME;
	}
	newattrs.ia_atime = actime;
	newattrs.ia_mtime = modtime;
	newattrs.ia_valid = ATTR_CTIME | ATTR_MTIME | ATTR_ATIME | flags;
	inode->i_dirt = 1;
	error = notify_change(inode, &newattrs);
	iput(inode);
	return error;
}

/*
 * access() needs to use the real uid/gid, not the effective uid/gid.
 * We do this by temporarily setting fsuid/fsgid to the wanted values
 */
asmlinkage int sys_access(const char * filename, int mode)
{
	struct inode * inode;
	int old_fsuid, old_fsgid;
	int res;

	if (mode != (mode & S_IRWXO))	/* where's F_OK, X_OK, W_OK, R_OK? */
		return -EINVAL;
	old_fsuid = current->fsuid;
	old_fsgid = current->fsgid;
	current->fsuid = current->uid;
	current->fsgid = current->gid;
	res = namei(filename,&inode);
	if (!res) {
		iput(inode);
	}
	current->fsuid = old_fsuid;
	current->fsgid = old_fsgid;
	return res;
}

asmlinkage int sys_chdir(const char * filename)
{
	struct inode * inode;
	int error;

	error = namei(filename,&inode);
	if (error)
		return error;
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	iput(current->fs->pwd);
	current->fs->pwd = inode;
	return (0);
}

asmlinkage int sys_fchdir(unsigned int fd)
{
	struct inode * inode;
	struct file * file;

	if (fd >= NR_OPEN || !(file = current->files->fd_array[fd]))
		return -EBADF;
	if (!(inode = file->f_inode))
		return -ENOENT;
	if (!S_ISDIR(inode->i_mode))
		return -ENOTDIR;
	iput(current->fs->pwd);
	current->fs->pwd = inode;
	atomic_inc(&inode->i_count);
	return (0);
}

asmlinkage int sys_chroot(const char * filename)
{
	struct inode * inode;
	int error;

	error = namei(filename,&inode);
	if (error)
		return error;
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	iput(current->fs->root);
	current->fs->root = inode;
	return (0);
}

asmlinkage int sys_fchmod(unsigned int fd, mode_t mode)
{
	struct inode * inode;
	struct file * file;
	struct iattr newattrs;

	if (fd >= NR_OPEN || !(file = current->files->fd_array[fd]))
		return -EBADF;
	if (!(inode = file->f_inode))
		return -ENOENT;
	if (IS_RDONLY(inode))
		return -EROFS;
	if (IS_IMMUTABLE(inode) || IS_APPEND(inode))
		return -EPERM;
	if (mode == (mode_t) -1)
		mode = inode->i_mode;
	newattrs.ia_mode = (mode & S_IALLUGO) | (inode->i_mode & ~S_IALLUGO);
	newattrs.ia_ctime = FS_CURRENT_TIME;
	newattrs.ia_valid = ATTR_MODE | ATTR_CTIME;
	inode->i_dirt = 1;
	return notify_change(inode, &newattrs);
}

asmlinkage int sys_chmod(const char * filename, mode_t mode)
{
	struct inode * inode;
	int error;
	struct iattr newattrs;

	error = namei(filename,&inode);
	if (error)
		return error;
	if (IS_RDONLY(inode)) {
		iput(inode);
		return -EROFS;
	}
	if (IS_IMMUTABLE(inode) || IS_APPEND(inode)) {
		iput(inode);
		return -EPERM;
	}
	if (mode == (mode_t) -1)
		mode = inode->i_mode;
	newattrs.ia_mode = (mode & S_IALLUGO) | (inode->i_mode & ~S_IALLUGO);
	newattrs.ia_ctime = FS_CURRENT_TIME;
	newattrs.ia_valid = ATTR_MODE | ATTR_CTIME;
	inode->i_dirt = 1;
	error = notify_change(inode, &newattrs);
	iput(inode);
	return error;
}

asmlinkage int sys_fchown(unsigned int fd, uid_t user, gid_t group)
{
	struct inode * inode;
	struct file * file;
	struct iattr newattrs;

	if (fd >= NR_OPEN || !(file = current->files->fd_array[fd]))
		return -EBADF;
	if (!(inode = file->f_inode))
		return -ENOENT;
	if (IS_RDONLY(inode))
		return -EROFS;
	if (IS_IMMUTABLE(inode) || IS_APPEND(inode))
		return -EPERM;
	if (user == (uid_t) -1)
		user = inode->i_uid;
	if (group == (gid_t) -1)
		group = inode->i_gid;
	newattrs.ia_mode = inode->i_mode;
	newattrs.ia_uid = user;
	newattrs.ia_gid = group;
	newattrs.ia_ctime = FS_CURRENT_TIME;
	newattrs.ia_valid =  ATTR_UID | ATTR_GID | ATTR_CTIME;
	/*
	 * If the owner has been changed, remove the setuid bit
	 */
	if (user != inode->i_uid && (inode->i_mode & S_ISUID)) {
		newattrs.ia_mode &= ~S_ISUID;
		newattrs.ia_valid |= ATTR_MODE;
	}
	/*
	 * If the group has been changed, remove the setgid bit
	 */
	if (group != inode->i_gid && (inode->i_mode & S_ISGID)) {
		newattrs.ia_mode &= ~S_ISGID;
		newattrs.ia_valid |= ATTR_MODE;
	}
	inode->i_dirt = 1;
	return notify_change(inode, &newattrs);
}

asmlinkage int sys_chown(const char * filename, uid_t user, gid_t group)
{
	struct inode * inode;
	int error;
	struct iattr newattrs;

	error = lnamei(filename,&inode);
	if (error)
		return error;
	if (IS_RDONLY(inode)) {
		iput(inode);
		return -EROFS;
	}
	if (IS_IMMUTABLE(inode) || IS_APPEND(inode)) {
		iput(inode);
		return -EPERM;
	}
	if (user == (uid_t) -1)
		user = inode->i_uid;
	if (group == (gid_t) -1)
		group = inode->i_gid;
	newattrs.ia_mode = inode->i_mode;
	newattrs.ia_uid = user;
	newattrs.ia_gid = group;
	newattrs.ia_ctime = FS_CURRENT_TIME;
	newattrs.ia_valid =  ATTR_UID | ATTR_GID | ATTR_CTIME;
	/*
	 * If the owner has been changed, remove the setuid bit
	 */
	if (user != inode->i_uid && (inode->i_mode & S_ISUID)) {
		newattrs.ia_mode &= ~S_ISUID;
		newattrs.ia_valid |= ATTR_MODE;
	}
	/*
	 * If the group has been changed, remove the setgid bit
	 */
	if (group != inode->i_gid && (inode->i_mode & S_ISGID)) {
		newattrs.ia_mode &= ~S_ISGID;
		newattrs.ia_valid |= ATTR_MODE;
	}
	inode->i_dirt = 1;
	error = notify_change(inode, &newattrs);
	iput(inode);
	return(error);
}

/**
 * 打开文件并返回其句柄。
 *  	flag:
 *			00 - no permissions needed
 *			01 - read-permission
 *			10 - write-permission
 *			11 - read-write
 *		mode:
 *			创建新文件时，指定访问者的权限，如S_IRWXU
 */
int do_open(const char * filename,int flags,int mode)
{
	struct inode * inode;
	struct file * f;
	int flag,error,fd;
	struct files_struct * files = current->files;
	int ret = 0;
	
	//分配一个可用的文件描述符
	f = get_empty_filp();
	if (!f)
		return -ENFILE;
	
	spin_lock(&files->file_lock);

	/**
	 * 警告:这里从3开始向用户返回句柄号，这是因为tty还没有完全实现的原因
	 * 请修改!!!!!!
	 */
	for(fd = 3; fd < files->max_fds; fd++) {
		if (files->fd_array[fd] == UNUSED_FILE) {
			break;
		}
	}
	//没有可用句柄了，打开文件太多。
	if (fd >= files->max_fds) {
		spin_unlock(&files->file_lock);
		return -EMFILE;
	}

	//暂时置为无效描述符，此描述符还不能被访问!!!
	files->fd_array[fd] = INVALIDATE_FILE;
	spin_unlock(&files->file_lock);
	
	flag = flags;
	f->f_flags = flags;
	f->f_mode = (flag+1) & O_ACCMODE;
	if (f->f_mode)
		flag++;
	//这两种模式都需要写权限
	if (flag & (O_TRUNC | O_CREAT))
		flag |= 2;

	//根据文件名，找到文件对应的inode
	error = open_namei(filename,flag,mode,&inode,NULL);
	if (!error && (f->f_mode & 2)) {//检查写权限
		error = get_write_access(inode);
		if (error)//不允许写此文件
			iput(inode);
	}
	if (error) {//失败了，重新将句柄设置为未用状态
		spin_lock(&files->file_lock);
		files->fd_array[fd] = UNUSED_FILE;
		spin_unlock(&files->file_lock);
		release_file(f);
		return error;
	}

	f->f_inode = inode;
	f->f_pos = 0;
	f->f_reada = 0;
	f->f_op = NULL;
	if (inode->i_op)
		f->f_op = inode->i_op->default_file_ops;
	if (f->f_op && f->f_op->open) {//回调文件系统的open回调
		error = f->f_op->open(inode,f);
		if (error) {//失败回退
			if (f->f_mode & 2) put_write_access(inode);
			iput(inode);
			release_file(f);
			
			spin_lock(&files->file_lock);
			files->fd_array[fd] = UNUSED_FILE;
			spin_unlock(&files->file_lock);

			return error;
		}
	}
	f->f_flags &= ~(O_CREAT | O_EXCL | O_NOCTTY | O_TRUNC);

	//打开成功，任务可以访问此文件描述符了。
	spin_lock(&files->file_lock);
	files->fd_array[fd] = f;
	spin_unlock(&files->file_lock);

	ret = fd;
	return ret;
}

/**
 * open系统调用
 *	filename:要打开的文件名
 *	flags:访问模式
 *	mode:创建文件需要的许可权限。
 */
asmlinkage int sys_open(const char * filename,int flags,int mode)
{
	char * tmp;
	int error;
#if BITS_PER_LONG != 32
		flags |= O_LARGEFILE;
#endif

	/**
	 * 从用户态获取文件名
	 */
	error = getname(filename, &tmp);
	if (error)
		return error;
	//这里执行真正的打开操作
	error = do_open(tmp,flags,mode);

	putname(tmp);
	return error;
}

/**
 * creat系统调用，创建一个新文件
 */
asmlinkage int sys_creat(const char * pathname, int mode)
{
	return sys_open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

/**
 * 关闭文件描述符
 */
int close_fp(struct file *filp)
{
	struct inode *inode;

	//安全检查
	if (atomic_read(&filp->f_count) == 0) {
		printk("VFS: Close: file count is 0\n");
		return 0;
	}
	inode = filp->f_inode;
	if (inode)//关闭文件锁
		fcntl_remove_locks(current, filp);
	//如果还有其他地方在引用此描述符，则退出
	if (!atomic_dec_and_test(&filp->f_count)) {
		return 0;
	}
	//调用文件系统回调，进行资源清理工作
	if (filp->f_op && filp->f_op->release)
		filp->f_op->release(inode,filp);

	filp->f_inode = NULL;
	if (filp->f_mode & 2) put_write_access(inode);
	//释放节点引用
	iput(inode);
	return 0;
}

/**
 * close系统调用，关闭一个打开的文件
 */
asmlinkage int sys_close(unsigned int fd)
{	
	struct file * filp;
	struct files_struct *files = current->files;

	//参数检查
	if (fd >= NR_OPEN)
		return -EBADF;
	//在锁的保护下，清空fd
	spin_lock(&files->file_lock);
	if (!(filp = files->fd_array[fd])) {
		spin_unlock(&files->file_lock);
		return -EBADF;
	}
	files->fd_array[fd] = NULL;
	spin_unlock(&files->file_lock);

	//关闭文件
	return (close_fp (filp));
}

asmlinkage mode_t sys_umask(mode_t mask)
{
	int old = current->fs->umask;

	current->fs->umask = mask & S_IRWXUGO;
	return (old);
}


