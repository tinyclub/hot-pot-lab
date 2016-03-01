#ccflags-y := -Iinclude/kapi/libc
obj-y := dummy.o buffer.o super.o inode.o file_table.o dcache.o filesystem.o \
		genhd.o namei.o open.o fcntl.o stat.o ioctl.o read_write.o select.o
obj-$(CONFIG_PROC_FS)	+= proc/
obj-$(CONFIG_MSDOS_FS)	+= msdos/
obj-$(CONFIG_UMSDOS_FS)	+= umsdos/
obj-$(CONFIG_EXT2_FS)	+= ext2/
#obj-$(CONFIG_YAFFS_FS)  += yaffs2/
