
#include <dim-sum/mem.h>
#include <fs/fs.h>
#include <fs/mount.h>
#include <fs/fcntl.h>
#include <fs/msdos_fs.h>
#include <dim-sum/sched.h>
#include <dim-sum/errno.h>
#include <asm/current.h>

#define ACC_MODE(x) ("\000\004\002\006"[(x)&O_ACCMODE])

/**
 * 判断当前用户是否有权限按指定方式访问文件
 */
int permission(struct inode * inode,int mask)
{
	return 0;
}

/**
 * 获得用户传入的文件名参数。
 * 注意:纯内核态应用不需要额外的复制过程
 */
int getname(const char * filename, char **result)
{
	//内核态中，不需要考虑文件名复制
	*result = (char *)filename;
	return 0;
}

/**
 * 释放内核文件名参数。
 * 注意:纯内核态应用不需要额外的释放过程
 */
void putname(char * name)
{
}

/**
 * 解析索引节点对象指定的符号链表。
 */
int follow_link(struct inode * dir, struct inode * inode,
	int flag, int mode, struct inode ** res_inode)
{
	if (!dir || !inode) {//参数检查
		iput(dir);
		iput(inode);
		*res_inode = NULL;
		return -ENOENT;
	}
	//不是链接文件，退出
	if (!inode->i_op || !inode->i_op->follow_link) {
		iput(dir);
		*res_inode = inode;
		return 0;
	}
	/**
	 * 处理链接文件
	 * xie.baoyou注意:这里没有处理递归，严重警告，修改!!!!!!
	 */
	return inode->i_op->follow_link(dir,inode,flag,mode,res_inode);
}

/**
 * 在目录中查找指定文件的节点。
 * 它主要调用具体文件系统的方法，同时也做一些额外的检查。
 * 		result:查找到的文件节点
 */
int lookup(struct inode * dir,const char * name, int len,
	struct inode ** result)
{
	struct super_block * sb;

	//参数检查
	*result = NULL;
	if (!dir)
		return -ENOENT;
	//返回上级目录
	if (len==2 && name[0] == '.' && name[1] == '.') {
		if (dir == current->fs->root) {//已经是根目录了，不能再向上回退
			*result = dir;
			return 0;
		} else if ((sb = dir->i_sb) && (dir == sb->s_mounted)) {//当前节点绑定了文件系统
			sb = dir->i_sb;
			iput(dir);
			/**
			 * 转到被绑定的根节点
			 * 注意:这里需要重点修改，有冲突!!!!!!
			 */
			dir = sb->s_covered;
			if (!dir)
				return -ENOENT;
			atomic_inc(&dir->i_count);
		}
	}
	//节点没有实现lookup回调，说明它不是一个目录
	if (!dir->i_op || !dir->i_op->lookup) {
		iput(dir);
		return -ENOTDIR;
	}
	//查找过程结束
	if (!len) {
		*result = dir;
		return 0;
	}
	//由文件系统完成它的lookup过程
	return dir->i_op->lookup(dir,name,len,result);
}


/**
 * 根据文件名，查找它所在的目录，以及它在目录中的名称
 */
static int dir_namei(const char * pathname, int * namelen, const char ** name,
	struct inode * base, struct inode ** res_inode)
{
	char c;
	const char * thisname;
	int len,error;
	struct inode * inode;
	struct fs_struct *fs = current->fs;

	*res_inode = NULL;
	read_lock(&fs->lock);
	if ((c = *pathname) == '/') {//从根目录开始查找
		iput(base);
		base = fs->root;
		pathname++;
		//增加目录和mount对象的引用计数，防止目录被意外删除，也防止文件系统被卸载。
		//atomic_inc(&fs->rootmnt->mnt_count);
		atomic_inc(&base->i_count);
	}
	else if (!base) {//没有指定基目录，从任务当前目录开始查找
		base = fs->pwd;
		//atomic_inc(&fs->pwdmnt->mnt_count);
		atomic_inc(&base->i_count);
	}
	read_unlock(&fs->lock);

	while (1) {
		thisname = pathname;
		//忽略所有'/'
		for(len = 0; (c = *(pathname++))&&(c != '/'); len++)
			/* nothing */ ;
		if (!c)//解析完毕，退出
			break;
		//递增目录引用计数
		atomic_inc(&base->i_count);
		//在base目录中查找文件
		error = lookup(base,thisname,len,&inode);
		if (error) {//搜索失败，退出
			iput(base);
			return error;
		}
		//处理符号链接
		error = follow_link(base,inode,0,0,&base);
		if (error)
			return error;
	}
	//最后解析的目录分量不是目录
	if (!base->i_op || !base->i_op->lookup) {
		iput(base);
		return -ENOTDIR;
	}
	//返回搜索到的目录和文件名
	*name = thisname;
	*namelen = len;
	*res_inode = base;
	return 0;
}


static int _namei(const char * pathname, struct inode * base,
	int follow_links, struct inode ** res_inode)
{
	const char * basename;
	int namelen,error;
	struct inode * inode;

	*res_inode = NULL;
	error = dir_namei(pathname,&namelen,&basename,base,&base);
	if (error)
		return error;
	atomic_inc(&base->i_count);	/* lookup uses up base */
	error = lookup(base,basename,namelen,&inode);
	if (error) {
		iput(base);
		return error;
	}
	if (follow_links) {
		error = follow_link(base,inode,0,0,&inode);
		if (error)
			return error;
	} else
		iput(base);
	*res_inode = inode;
	return 0;
}

/*
 *	namei()
 *
 * is used by most simple commands to get the inode of a specified name.
 * Open, link etc use their own routines, but this is enough for things
 * like 'chmod' etc.
 */
int namei(const char * pathname, struct inode ** res_inode)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = _namei(tmp,NULL,1,res_inode);
		putname(tmp);
	}
	return error;
}


/**
 * 获得文件的写访问权限
 */
int get_write_access(struct inode * inode)
{
	inode->i_wcount++;
	return 0;
}

/**
 * 释放文件的写访问权限
 */
void put_write_access(struct inode * inode)
{
	inode->i_wcount--;
}

int lnamei(const char * pathname, struct inode ** res_inode)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = _namei(tmp,NULL,0,res_inode);
		putname(tmp);
	}
	return error;
}

/**
 * 根据文件名，找到文件对应的inode，用于sys_open系统调用
 * 这是sys_open调用的主要处理过程
 * 注意!这里的flag标志与sys_open系统调用不同:
 *	 	00 - no permissions needed
 *		01 - read permission needed
 *		10 - write permission needed
 *		11 - read/write permissions needed
 */
int open_namei(const char * pathname, int flag, int mode,
	struct inode ** res_inode, struct inode * base)
{
	const char * basename;
	int namelen,error;
	struct inode * dir, *inode;

	mode &= S_IALLUGO & ~current->fs->umask;
	mode |= S_IFREG;
	//查找文件所在的目录，及它在目录中的名称
	error = dir_namei(pathname,&namelen,&basename,base,&dir);
	if (error)
		return error;
	//最后一级是目录
	if (!namelen) {			/* special case: '/usr/' etc */
		if (flag & 2) {//试图写目录
			iput(dir);
			return -EISDIR;
		}
		//判断对目录的操作是否合法
		if ((error = permission(dir,ACC_MODE(flag))) != 0) {
			iput(dir);
			return error;
		}
		//返回目录节点
		*res_inode=dir;
		return 0;
	}
	//添加目录的引用计数
	atomic_inc(&dir->i_count);	
	if (flag & O_CREAT) {//创建文件
		//获得节点的信号量
		down(&dir->i_sem);
		//再次查找文件是否存在，因为其他地方可能已经创建了同名文件
		error = lookup(dir,basename,namelen,&inode);
		if (!error) {//文件已经存在
			if (flag & O_EXCL) {//调用者要求确保文件被创建，而我们并没有创建文件
				iput(inode);
				error = -EEXIST;
			}
		} else if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0)//没有操作目录的权限
			;	/* error is already set! */
		else if (!dir->i_op || !dir->i_op->create)//文件系统不支持创建文件
			error = -EACCES;
		else if (IS_RDONLY(dir))//只读目录
			error = -EROFS;
		else {
			//调用文件系统的回调，创建一个文件节点
			atomic_inc(&dir->i_count);		/* create eats the dir */
			error = dir->i_op->create(dir,basename,namelen,mode,res_inode);
			up(&dir->i_sem);//释放信号量
			iput(dir);
			return error;
		}
		up(&dir->i_sem);//释放信号量
	} else {//仅仅打开而不创建文件 
		error = lookup(dir,basename,namelen,&inode);
	}
	
	if (error) {
		iput(dir);
		return error;
	}
	//处理软链接，打开软链接指向的目标文件
	error = follow_link(dir,inode,flag,mode,&inode);
	if (error)
		return error;
	//目录是不能被写的
	if (S_ISDIR(inode->i_mode) && (flag & 2)) {
		iput(inode);
		return -EISDIR;
	}
	//判断权限
	if ((error = permission(inode,ACC_MODE(flag))) != 0) {
		iput(inode);
		return error;
	}
	if (S_ISBLK(inode->i_mode) || S_ISCHR(inode->i_mode)) {//设备文件
		if (IS_NODEV(inode)) {//不允许访问
			iput(inode);
			return -EACCES;
		}
		//设备文件不允许被截断
		flag &= ~O_TRUNC;
	} else {
		//只读节点，不允许写
		if (IS_RDONLY(inode) && (flag & 2)) {
			iput(inode);
			return -EROFS;
		}
	}
	/*
	 * 节点只能被添加数据
	 */
	if (IS_APPEND(inode) && ((flag & 2) && !(flag & O_APPEND))) {
		iput(inode);
		return -EPERM;
	}
	if (flag & O_TRUNC) {//处理截数
		struct iattr newattrs;

		if ((error = get_write_access(inode))) {
			iput(inode);
			return error;
		}
		newattrs.ia_size = 0;
		newattrs.ia_valid = ATTR_SIZE;
		//调用文件系统的回调，处理节点长度变化
		if ((error = notify_change(inode, &newattrs))) {
			put_write_access(inode);
			iput(inode);
			return error;
		}
		inode->i_size = 0;
		//调用文件系统的回调，截断文件
		if (inode->i_op && inode->i_op->truncate)
			inode->i_op->truncate(inode);
		//设置文件脏标记
		inode->i_dirt = 1;
		put_write_access(inode);
	}
	*res_inode = inode;
	return 0;
}


static int do_link(struct inode * oldinode, const char * newname)
{
	struct inode * dir;
	const char * basename;
	int namelen, error;

	error = dir_namei(newname,&namelen,&basename,NULL,&dir);
	if (error) {
		iput(oldinode);
		return error;
	}
	if (!namelen) {
		iput(oldinode);
		iput(dir);
		return -EPERM;
	}
	if (IS_RDONLY(dir)) {
		iput(oldinode);
		iput(dir);
		return -EROFS;
	}
	if (dir->i_dev != oldinode->i_dev) {
		iput(dir);
		iput(oldinode);
		return -EXDEV;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		iput(oldinode);
		return error;
	}
	/*
	 * A link to an append-only or immutable file cannot be created
	 */
	if (IS_APPEND(oldinode) || IS_IMMUTABLE(oldinode)) {
		iput(dir);
		iput(oldinode);
		return -EPERM;
	}
	if (!dir->i_op || !dir->i_op->link) {
		iput(dir);
		iput(oldinode);
		return -EPERM;
	}
	atomic_inc(&dir->i_count);
	down(&dir->i_sem);
	error = dir->i_op->link(oldinode, dir, basename, namelen);
	up(&dir->i_sem);
	iput(dir);
	return error;
}

asmlinkage int sys_link(const char * oldname, const char * newname)
{
	int error;
	char * to;
	struct inode * oldinode;

	error = namei(oldname, &oldinode);
	if (error)
		return error;
	error = getname(newname,&to);
	if (error) {
		iput(oldinode);
		return error;
	}
	error = do_link(oldinode,to);
	putname(to);
	return error;
}


int do_mknod(const char * filename, int mode, dev_t dev)
{
	const char * basename;
	int namelen, error;
	struct inode * dir;

	mode &= ~current->fs->umask;
	error = dir_namei(filename,&namelen,&basename, NULL, &dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	if (!dir->i_op || !dir->i_op->mknod) {
		iput(dir);
		return -EPERM;
	}
	atomic_inc(&dir->i_count);
	down(&dir->i_sem);
	error = dir->i_op->mknod(dir,basename,namelen,mode,dev);
	up(&dir->i_sem);
	iput(dir);
	return error;
}

asmlinkage int sys_mknod(const char * filename, int mode, dev_t dev)
{
	int error;
	char * tmp;

#ifdef xbyd_debug
	if (S_ISDIR(mode) || (!S_ISFIFO(mode) && !fsuser()))
		return -EPERM;
#else
if (S_ISDIR(mode))
	return -EPERM;
#endif
	switch (mode & S_IFMT) {
	case 0:
		mode |= S_IFREG;
		break;
	case S_IFREG: case S_IFCHR: case S_IFBLK: case S_IFIFO: case S_IFSOCK:
		break;
	default:
		return -EINVAL;
	}
	error = getname(filename,&tmp);
	if (!error) {
		error = do_mknod(tmp,mode,dev);
		putname(tmp);
	}
	return error;
}

static int do_mkdir(const char * pathname, int mode)
{
	const char * basename;
	int namelen, error;
	struct inode * dir;

	error = dir_namei(pathname,&namelen,&basename,NULL,&dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	if (!dir->i_op || !dir->i_op->mkdir) {
		iput(dir);
		return -EPERM;
	}
	atomic_inc(&dir->i_count);
	down(&dir->i_sem);
	error = dir->i_op->mkdir(dir, basename, namelen, mode & 0777 & ~current->fs->umask);
	up(&dir->i_sem);
	iput(dir);
	return error;
}

asmlinkage int sys_mkdir(const char * pathname, int mode)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = do_mkdir(tmp,mode);
		putname(tmp);
	}
	return error;
}


static int do_rename(const char * oldname, const char * newname)
{
	struct inode * old_dir, * new_dir;
	const char * old_base, * new_base;
	int old_len, new_len, error;

	error = dir_namei(oldname,&old_len,&old_base,NULL,&old_dir);
	if (error)
		return error;
	if ((error = permission(old_dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(old_dir);
		return error;
	}
	if (!old_len || (old_base[0] == '.' &&
	    (old_len == 1 || (old_base[1] == '.' &&
	     old_len == 2)))) {
		iput(old_dir);
		return -EPERM;
	}
	error = dir_namei(newname,&new_len,&new_base,NULL,&new_dir);
	if (error) {
		iput(old_dir);
		return error;
	}
	if ((error = permission(new_dir,MAY_WRITE | MAY_EXEC)) != 0){
		iput(old_dir);
		iput(new_dir);
		return error;
	}
	if (!new_len || (new_base[0] == '.' &&
	    (new_len == 1 || (new_base[1] == '.' &&
	     new_len == 2)))) {
		iput(old_dir);
		iput(new_dir);
		return -EPERM;
	}
	if (new_dir->i_dev != old_dir->i_dev) {
		iput(old_dir);
		iput(new_dir);
		return -EXDEV;
	}
	if (IS_RDONLY(new_dir) || IS_RDONLY(old_dir)) {
		iput(old_dir);
		iput(new_dir);
		return -EROFS;
	}
	/*
	 * A file cannot be removed from an append-only directory
	 */
	if (IS_APPEND(old_dir)) {
		iput(old_dir);
		iput(new_dir);
		return -EPERM;
	}
	if (!old_dir->i_op || !old_dir->i_op->rename) {
		iput(old_dir);
		iput(new_dir);
		return -EPERM;
	}
	atomic_inc(&new_dir->i_count);
	down(&new_dir->i_sem);
	error = old_dir->i_op->rename(old_dir, old_base, old_len, 
		new_dir, new_base, new_len);
	up(&new_dir->i_sem);
	iput(new_dir);
	return error;
}

asmlinkage int sys_rename(const char * oldname, const char * newname)
{
	int error;
	char * from, * to;

	error = getname(oldname,&from);
	if (!error) {
		error = getname(newname,&to);
		if (!error) {
			error = do_rename(from,to);
			putname(to);
		}
		putname(from);
	}
	return error;
}


static int do_rmdir(const char * name)
{
	const char * basename;
	int namelen, error;
	struct inode * dir;

	error = dir_namei(name,&namelen,&basename,NULL,&dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	/*
	 * A subdirectory cannot be removed from an append-only directory
	 */
	if (IS_APPEND(dir)) {
		iput(dir);
		return -EPERM;
	}
	if (!dir->i_op || !dir->i_op->rmdir) {
		iput(dir);
		return -EPERM;
	}
	return dir->i_op->rmdir(dir,basename,namelen);
}

asmlinkage int sys_rmdir(const char * pathname)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = do_rmdir(tmp);
		putname(tmp);
	}
	return error;
}


static int do_symlink(const char * oldname, const char * newname)
{
	struct inode * dir;
	const char * basename;
	int namelen, error;

	error = dir_namei(newname,&namelen,&basename,NULL,&dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	if (!dir->i_op || !dir->i_op->symlink) {
		iput(dir);
		return -EPERM;
	}
	atomic_inc(&dir->i_count);
	down(&dir->i_sem);
	error = dir->i_op->symlink(dir,basename,namelen,oldname);
	up(&dir->i_sem);
	iput(dir);
	return error;
}

asmlinkage int sys_symlink(const char * oldname, const char * newname)
{
	int error;
	char * from, * to;

	error = getname(oldname,&from);
	if (!error) {
		error = getname(newname,&to);
		if (!error) {
			error = do_symlink(from,to);
			putname(to);
		}
		putname(from);
	}
	return error;
}


static int do_unlink(const char * name)
{
	const char * basename;
	int namelen, error;
	struct inode * dir;

	error = dir_namei(name,&namelen,&basename,NULL,&dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -EPERM;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if ((error = permission(dir,MAY_WRITE | MAY_EXEC)) != 0) {
		iput(dir);
		return error;
	}
	/*
	 * A file cannot be removed from an append-only directory
	 */
	if (IS_APPEND(dir)) {
		iput(dir);
		return -EPERM;
	}
	if (!dir->i_op || !dir->i_op->unlink) {
		iput(dir);
		return -EPERM;
	}
	return dir->i_op->unlink(dir,basename,namelen);
}

asmlinkage int sys_unlink(const char * pathname)
{
	int error;
	char * tmp;

	error = getname(pathname,&tmp);
	if (!error) {
		error = do_unlink(tmp);
		putname(tmp);
	}
	return error;
}

