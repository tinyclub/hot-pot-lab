#define MAJOR_NR  SD_MAJOR

#include <base/string.h>
#include <dim-sum/major.h>
#include <dim-sum/boot_allotter.h>
#include <fs/fs.h>
#include <fs/locks.h>
#include <fs/blk.h>

static int sd_blocksizes[2] = {0, 0};


#define SD_MINOR	1

extern ulong mmc_bwrite(int dev_num, unsigned long start, unsigned long blkcnt, const void *src);
extern ulong mmc_bread(int dev_num, unsigned long start, unsigned long blkcnt, void *dst);

static void do_sd_request(void)
{
	sector_t block_start;
	sector_t block_count;

repeat:
	INIT_REQUEST;
	block_start = (CURRENT->sector);
	block_count = CURRENT->current_nr_sectors;

	if (CURRENT->cmd == WRITE) {
		(void ) mmc_bwrite(0, 
				  block_start,
				  block_count,
			      CURRENT->buffer);
	} else if (CURRENT->cmd == READ) {
		(void) mmc_bread(0, 
				  block_start,
				  block_count,
				  CURRENT->buffer);
	} else
		panic("SD-DISK: unknown SD disk command !\n");
	end_request(1);
	goto repeat;
}

static struct file_operations sd_fops = {
	NULL,			/* lseek - default */
	block_read,		/* read - general block-dev read */
	block_write,		/* write - general block-dev write */
	NULL,			/* readdir - bad */
	NULL,			/* select */
	NULL,			/* ioctl */
	NULL,			/* mmap */
	NULL,			/* no special open code */
	NULL,			/* no special release code */
	block_fsync		/* fsync */
};

/*
 * ≥ı ºªØsd-disk
 */
void sd_init(void)
{
	int i;
	if (register_blkdev(SD_MAJOR,"sd",&sd_fops)) {
		printk("SD-DISK: Unable to get major %d.\n", SD_MAJOR);
		return;
	}
	blk_dev[SD_MAJOR].request_fn = DEVICE_REQUEST;

	for(i=0;i<2;i++) sd_blocksizes[i] = 512;
	blksize_size[MAJOR_NR] = sd_blocksizes;
	ROOT_DEV = ((SD_MAJOR << 8) | SD_MINOR);
}


