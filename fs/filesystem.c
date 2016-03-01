#include <linux/linkage.h>
#include <fs/fs.h>
#include <fs/ext2_fs.h>
#include <fs/msdos_fs.h>
#include <fs/proc_fs.h>
#include <dim-sum/syscall.h>

extern void device_setup(void);

extern struct super_block *UMSDOS_read_super(
	struct super_block *s,
	void *data,
	int silent);
/**
 * 文件系统初始化
 */
asmlinkage int sys_setup(void)
{
	static int callable = 1;

	//防止多次初始化
	if (!callable)
		return -1;
	callable = 0;

	//初始化设备
	device_setup();

	//注册文件系统
#ifdef CONFIG_MINIX_FS
	register_filesystem(&(struct file_system_type)
		{minix_read_super, "minix", 1, NULL});
#endif

#ifdef CONFIG_EXT_FS
	register_filesystem(&(struct file_system_type)
		{ext_read_super, "ext", 1, NULL});
#endif

#ifdef CONFIG_EXT2_FS
	register_filesystem(&(struct file_system_type)
		{ext2_read_super, "ext2", 1, NULL});
#endif

#ifdef CONFIG_XIA_FS
	register_filesystem(&(struct file_system_type)
		{xiafs_read_super, "xiafs", 1, NULL});
#endif
#ifdef CONFIG_UMSDOS_FS
	register_filesystem(&(struct file_system_type)
	{UMSDOS_read_super,	"umsdos",	1, NULL});
#endif

#ifdef CONFIG_MSDOS_FS
	register_filesystem(&(struct file_system_type)
		{msdos_read_super, "msdos", 1, NULL});
#endif

#ifdef CONFIG_PROC_FS
	register_filesystem(&(struct file_system_type)
		{proc_read_super, "proc", 0, NULL});
#endif

#ifdef CONFIG_NFS_FS
	register_filesystem(&(struct file_system_type)
		{nfs_read_super, "nfs", 0, NULL});
#endif

#ifdef CONFIG_ISO9660_FS
	register_filesystem(&(struct file_system_type)
		{isofs_read_super, "iso9660", 1, NULL});
#endif

#ifdef CONFIG_SYSV_FS
	register_filesystem(&(struct file_system_type)
		{sysv_read_super, "xenix", 1, NULL});

	register_filesystem(&(struct file_system_type)
		{sysv_read_super, "sysv", 1, NULL});

	register_filesystem(&(struct file_system_type)
		{sysv_read_super, "coherent", 1, NULL});
#endif

#ifdef CONFIG_HPFS_FS
	register_filesystem(&(struct file_system_type)
		{hpfs_read_super, "hpfs", 1, NULL});
#endif

	//挂载root设备
	mount_root();
	return 0;
}

