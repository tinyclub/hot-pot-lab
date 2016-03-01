
#ifndef __PHOENIX_FILE_H
#define __PHOENIX_FILE_H

#include <asm/atomic.h>
#include <linux/posix_types.h>
#include <linux/compiler.h>
#include <dim-sum/spinlock.h>

#define NR_OPEN_DEFAULT 1024

/**
 * 进程当前打开的文件。
 */
struct files_struct {
		/**
		 * 保护该表的读写自旋锁。
		 */
        spinlock_t file_lock;     /* Protects all the below members.  Nests inside tsk->alloc_lock */
		/**
		 * 文件对象的当前最大编号。
		 */
        int max_fds;
		/**
		 * 文件描述符的当前最大编号。
		 */
        int max_fdset;
		/**
		 * 上次分配的最大文件描述符加1.
		 */
        int next_fd;
		/**
		 * 打开文件描述符的指针。
		 */
        fd_set *open_fds;
		/**
		 * 文件对象指针的初始化数组。
		 */
        struct file * fd_array[__FD_SETSIZE];
};
		
#endif
