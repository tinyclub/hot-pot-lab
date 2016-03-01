
#include <base/string.h>
#include <dim-sum/mem.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/msdos_fs.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <dim-sum/errno.h>

/**
 * readdir系统调用
 */
asmlinkage int sys_readdir(unsigned int fd, struct dirent * dirent, unsigned int count)
{
	struct file * file;
	struct inode * inode;
	int ret = -EBADF; 

	//将文件句柄转换为文件描述符，并获得其引用
	file = fd2file(fd);
	if (!file)
		return -EBADF;

	//参数检查
	if (!(inode = file->f_inode))
		goto out;

	ret = -ENOTDIR;
	if (file->f_op && file->f_op->readdir) {
		int size = count * sizeof(struct dirent);

		//验证输出缓冲区的有效性
		ret = verify_area(VERIFY_WRITE, dirent, size);

		//调用文件系统的readdir回调，读取目录中的文件
		if (!ret)
			ret = file->f_op->readdir(inode,file,dirent,count);
	}

out:
	put_file(file);//释放文件描述符的引用
	return ret;

}

/**
 * lseek系统调用
 */
asmlinkage int sys_lseek(unsigned int fd, off_t offset, unsigned int origin)
{
	struct file * file;
	int tmp = -1;
	int ret = -EBADF; 

	//将文件句柄转换为文件描述符，并获得其引用
	file = fd2file(fd);
	if (!file)
		return -EBADF;

	//参数检查
	if (!(file->f_inode))
		goto out;

	ret = -EINVAL;
	if (origin > 2)
		goto out;
	if (file->f_op && file->f_op->lseek) {
		ret = file->f_op->lseek(file->f_inode,file,offset,origin);
		goto out;
	}
	
	//计算读写位置
	switch (origin) {
		case 0:
			tmp = offset;
			break;
		case 1:
			tmp = file->f_pos + offset;
			break;
		case 2:
			if (!file->f_inode)
				return -EINVAL;
			tmp = file->f_inode->i_size + offset;
			break;
	}
	if (tmp < 0) {
		ret = -EINVAL;
		goto out;
	}

	//设置位置
	if (tmp != file->f_pos) {
		file->f_pos = tmp;
		file->f_reada = 0;//重置预读位置
		file->f_version = ++event;
	}
	ret = file->f_pos;

out:
	put_file(file);//释放文件描述符的引用
	return ret;
}

/**
 * llseek系统调用
 */
asmlinkage int sys_llseek(unsigned int fd, unsigned long offset_high,
			  unsigned long offset_low, loff_t * result,
			  unsigned int origin)
{
	struct file * file;
	loff_t tmp = -1;
	loff_t offset;
	int ret = -EBADF;

	//将文件句柄转换为文件描述符，并获得其引用
	file = fd2file(fd);
	if (!file)
		return -EBADF;
	//参数检查
	if (!(file->f_inode))
		goto out;

	ret = -EINVAL;
	if (origin > 2)
		goto out;
	
	if ((ret = verify_area(VERIFY_WRITE, result, sizeof(loff_t))))
		goto out;
	
	offset = (loff_t) (((unsigned long long) offset_high << 32) | offset_low);
	//计算新位置
	switch (origin) {
		case 0:
			tmp = offset;
			break;
		case 1:
			tmp = file->f_pos + offset;
			break;
		case 2:
			//这里不用再判断f_inode的有效性了
			tmp = file->f_inode->i_size + offset;
			break;
	}
	if (tmp < 0) {
		ret = -EINVAL;
		goto out;
	}

	//设置读写位置
	file->f_pos = tmp;
	//重置预读位置
	file->f_reada = 0;
	file->f_version = ++event;
	memcpy_tofs(result, &file->f_pos, sizeof(loff_t));

	ret = 0;

out:
	put_file(file);//释放文件描述符的引用
	return ret;
}

/**
 * 读文件系统调用
 */
asmlinkage int sys_read(unsigned int fd,char * buf,unsigned int count)
{
	struct file * file;
	struct inode * inode;
	int ret = -EBADF;

	//将文件句柄转换为文件描述符，并获得其引用
	file = fd2file(fd);
	if (!file)
		return -EBADF;

	//参数检查
	if (!(inode = file->f_inode))
		goto out;
	if (!(file->f_mode & 1))
		goto out;
	if (!file->f_op || !file->f_op->read)
		goto out;

	ret = 0;
	if (!count)
		goto out;

	//缓冲区有效性验证，暂时不需要验证
	ret = verify_area(VERIFY_WRITE,buf,count);
	if (ret)
		goto out;
	//直接调用文件系统的read回调，读取数据到缓冲区中。
	ret = file->f_op->read(inode,file,buf,count);

out:
	put_file(file);//释放文件描述符的引用
	return ret;
}

/**
 * 写文件系统调用
 */
asmlinkage int sys_write(unsigned int fd,const char * buf,unsigned int count)
{
	struct file * file;
	struct inode * inode;
	int ret = -EBADF;

	int written;

	//将文件句柄转换为文件描述符，并获得其引用
	file = fd2file(fd);
	if (!file)
		return -EBADF;

	//参数检查
	if (!(inode = file->f_inode))
		goto out;
	if (!(file->f_mode & 2))
		goto out;
	if (!file->f_op || !file->f_op->write)
		goto out;

	ret = 0;
	if (!count)
		goto out;

	//缓冲区有效性验证，暂时不需要验证
	ret = verify_area(VERIFY_READ,buf,count);
	if (ret)
		goto out;

	//直接调用文件系统的write回调，写数据到缓冲区中。
	written = file->f_op->write(inode,file,buf,count);
	//数据已经被修改，不能再设置其S_ISUID和S_ISGID标志。
	if (written > 0 && !suser() && (inode->i_mode & (S_ISUID | S_ISGID))) {
		struct iattr newattrs;
		newattrs.ia_mode = inode->i_mode & ~(S_ISUID | S_ISGID);
		newattrs.ia_valid = ATTR_MODE;
		//修改节点属性
		notify_change(inode, &newattrs);
	}
	return written;

out:
	put_file(file);//释放文件描述符的引用
	return ret;
}
