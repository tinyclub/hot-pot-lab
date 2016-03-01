#ifndef _LINUX_BLKDEV_H
#define _LINUX_BLKDEV_H
#include <dim-sum/major.h>
#include <dim-sum/semaphore.h>
#include <dim-sum/wait.h>
/*
 * Ok, this is an expanded form so that we can use the same
 * request for paging requests when that is implemented. In
 * paging, 'bh' is NULL, and the semaphore is used to wait
 * for read/write completion.
 */
struct request {
	int dev;		/* -1 if no request */
	int cmd;		/* READ or WRITE */
	int errors;
	unsigned long sector;
	unsigned long nr_sectors;
	unsigned long current_nr_sectors;
	char * buffer;
	semaphore sem;
	struct buffer_head * bh;
	struct buffer_head * bhtail;
	struct request * next;
};

struct blk_dev_struct {
	void (*request_fn)(void);
	struct request * current_request;
};

extern struct blk_dev_struct blk_dev[MAX_BLKDEV];
extern int * blksize_size[MAX_BLKDEV];
extern int * hardsect_size[MAX_BLKDEV];


//extern struct sec_size * blk_sec[MAX_BLKDEV];
extern struct blk_dev_struct blk_dev[MAX_BLKDEV];
extern wait_queue_head_t wait_for_request;

#endif
