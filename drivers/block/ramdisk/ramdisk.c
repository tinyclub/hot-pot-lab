#define MAJOR_NR  MEM_MAJOR

#include <base/string.h>
#include <dim-sum/major.h>
#include <dim-sum/boot_allotter.h>
#include <fs/fs.h>
#include <fs/locks.h>
#include <fs/blk.h>


char	*rd_start;
int	rd_length = 0;
static int rd_blocksizes[2] = {0, 0};


#define RAMDISK_MINOR	1


static void do_rd_request(void)
{
	int	len;
	char	*addr;

repeat:
	INIT_REQUEST;
	addr = rd_start + (CURRENT->sector << 9);
	len = CURRENT->current_nr_sectors << 9;

	if ((MINOR(CURRENT->dev) != RAMDISK_MINOR) ||
	    (addr+len > rd_start+rd_length)) {
		end_request(0);
		goto repeat;
	}
	if (CURRENT-> cmd == WRITE) {
		(void ) memcpy(addr,
			      CURRENT->buffer,
			      len);
	} else if (CURRENT->cmd == READ) {
		(void) memcpy(CURRENT->buffer, 
			      addr,
			      len);
	} else
		panic("RAMDISK: unknown RAM disk command !\n");
	end_request(1);
	goto repeat;
}

static struct file_operations rd_fops = {
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
 * ≥ı ºªØramdisk
 */
void rd_init(int length)
{
	int	i;
	char	*cp;

	if (register_blkdev(MEM_MAJOR,"rd",&rd_fops)) {
		printk("RAMDISK: Unable to get major %d.\n", MEM_MAJOR);
		return;
	}
	blk_dev[MEM_MAJOR].request_fn = DEVICE_REQUEST;
	rd_start = (char *) AllocPermanentBootMemory(BOOT_MEMORY_MODULE_IO, length, 0);
	rd_length = length;
	cp = rd_start;
	for (i=0; i < length; i++)
		*cp++ = '\0';

	for(i=0;i<2;i++) rd_blocksizes[i] = 1024;
	blksize_size[MAJOR_NR] = rd_blocksizes;
}


extern int FormatRamWithFAT(char	*rd_start, int rd_length);


static void do_load(void)
{
#ifndef CONFIG_SD
	ROOT_DEV = ((MEM_MAJOR << 8) | RAMDISK_MINOR);
#endif

	FormatRamWithFAT(rd_start, rd_length);

	return;
}


/*
 * If the root device is the RAM disk, try to load it.
 * In order to do this, the root device is originally set to the
 * floppy, and we later change it to be RAM disk.
 */
void rd_load(void)
{
	/* If no RAM disk specified, give up early. */
	if (!rd_length)
		return;
	printk("RAMDISK: %d bytes, starting at 0x%p\n",
			rd_length, rd_start);


	

	/* for Slackware install disks */
	printk(KERN_NOTICE "VFS: Insert ramdisk floppy and press ENTER\n");

	do_load();
}

