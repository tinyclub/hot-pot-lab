#include <base/string.h>
#include <dim-sum/major.h>
#include <fs/blk.h>
#include <fs/fs.h>
#include <fs/fcntl.h>
#include <asm/page.h>
#include <linux/errno.h>
#include <fs/locks.h>
#include <dim-sum/wait.h>

int * hardsect_size[MAX_BLKDEV] = { NULL, NULL, };


/*
 * used to wait on when there are no free requests
 */
DECLARE_WAIT_QUEUE_HEAD(wait_for_request);


int read_ahead[MAX_BLKDEV] = {0, };
/*
 * The request-struct contains all necessary data
 * to load a nr of sectors into memory
 */
static struct request all_requests[NR_REQUEST];

struct kernel_stat kstat;
/*
 * blk_size contains the size of all block-devices in units of 1024 byte
 * sectors:
 *
 * blk_size[MAJOR][MINOR]
 *
 * if (!blk_size[MAJOR]) then no minor size checking is done.
 */
int * blk_size[MAX_BLKDEV] = { NULL, NULL, };


/* RO fail safe mechanism */

static long ro_bits[MAX_BLKDEV][8];

int is_read_only(int dev)
{
	int minor,major;

	major = MAJOR(dev);
	minor = MINOR(dev);
	if (major < 0 || major >= MAX_BLKDEV) return 0;
	return ro_bits[major][minor >> 5] & (1 << (minor & 31));
}


/*
 * "plug" the device if there are no outstanding requests: this will
 * force the transfer to start only after we have put all the requests
 * on the list.
 */
static void plug_device(struct blk_dev_struct * dev, struct request * plug)
{
	unsigned long flags;

	plug->dev = -1;
	plug->cmd = -1;
	plug->next = NULL;

	local_irq_save(flags);
	if (!dev->current_request)
		dev->current_request = plug;
	local_irq_restore(flags);
}


/*
 * remove the plug and let it rip..
 */
static void unplug_device(struct blk_dev_struct * dev)
{
	struct request * req;
	unsigned long flags;

	local_irq_save(flags);

	req = dev->current_request;
	if (req && req->dev == -1 && req->cmd == -1) {
		dev->current_request = req->next;
		(dev->request_fn)();
	}
	local_irq_restore(flags);
}

/*
 * look for a free request in the first N entries.
 * NOTE: interrupts must be disabled on the way in, and will still
 *       be disabled on the way out.
 */
static inline struct request * get_request(int n, int dev)
{
	static struct request *prev_found = NULL, *prev_limit = NULL;
	register struct request *req, *limit;

	if (n <= 0)
		panic("get_request(%d): impossible!\n", n);

	limit = all_requests + n;
	if (limit != prev_limit) {
		prev_limit = limit;
		prev_found = all_requests;
	}
	req = prev_found;
	for (;;) {
		req = ((req > all_requests) ? req : limit) - 1;
		if (req->dev < 0)
			break;
		if (req == prev_found)
			return NULL;
	}
	prev_found = req;
	req->dev = dev;
	return req;
}


/*
 * add-request adds a request to the linked list.
 * It disables interrupts so that it can muck with the
 * request-lists in peace.
 */
static void add_request(struct blk_dev_struct * dev, struct request * req)
{
	struct request * tmp;
	short		 disk_index;
	unsigned long flags;

	switch (MAJOR(req->dev)) {
		case SCSI_DISK_MAJOR:	disk_index = (MINOR(req->dev) & 0x0070) >> 4;
					if (disk_index < 4)
						kstat.dk_drive[disk_index]++;
					break;
		case HD_MAJOR:
		case XT_DISK_MAJOR:	disk_index = (MINOR(req->dev) & 0x0040) >> 6;
					kstat.dk_drive[disk_index]++;
					break;
		case IDE1_MAJOR:	disk_index = ((MINOR(req->dev) & 0x0040) >> 6) + 2;
					kstat.dk_drive[disk_index]++;
		default:		break;
	}

	req->next = NULL;
	local_irq_save(flags);

	if (req->bh)
		mark_buffer_clean(req->bh);
	if (!(tmp = dev->current_request)) {
		dev->current_request = req;
		(dev->request_fn)();
		
		local_irq_restore(flags);

		return;
	}
	for ( ; tmp->next ; tmp = tmp->next) {
		if ((IN_ORDER(tmp,req) ||
		    !IN_ORDER(tmp,tmp->next)) &&
		    IN_ORDER(req,tmp->next))
			break;
	}
	req->next = tmp->next;
	tmp->next = req;

/* for SCSI devices, call request_fn unconditionally */
	if (scsi_major(MAJOR(req->dev)))
		(dev->request_fn)();

	local_irq_restore(flags);
}


/*
 * wait until a free request in the first N entries is available.
 */
static struct request * __get_request_wait(int n, int dev)
{
	register struct request *req;
	//xby_deubg
	wait_queue_t wait;
	unsigned long flags;

	init_waitqueue_entry(&wait, current);
	add_wait_queue(&wait_for_request, &wait);
	
	for (;;) {
		unplug_device(MAJOR(dev)+blk_dev);
		set_current_state(TASK_INTERRUPTIBLE);
		
		local_irq_save(flags);
		req = get_request(n, dev);
		local_irq_restore(flags);

		if (req)
			break;
		TaskSwitch();
	}
	remove_wait_queue(&wait_for_request, &wait);
	current->state = TASK_RUNNING;
	return req;
}


static void make_request(int major,int rw, struct buffer_head * bh)
{
	unsigned int sector, count;
	struct request * req;
	int rw_ahead, max_req;
	unsigned long flags;

/* WRITEA/READA is special case - it is not really needed, so if the */
/* buffer is locked, we just forget about it, else it's a normal read */
	rw_ahead = (rw == READA || rw == WRITEA);
	if (rw_ahead) {
		if (bh->b_lock)
			return;
		if (rw == READA)
			rw = READ;
		else
			rw = WRITE;
	}
	
	if (rw!=READ && rw!=WRITE) {
		printk("Bad block dev command, must be R/W/RA/WA\n");
		return;
	}
	count = bh->b_size >> 9;
	sector = bh->b_blocknr * count;
	if (blk_size[major])
		if (blk_size[major][MINOR(bh->b_dev)] < (sector + count)>>1) {
			bh->b_dirt = bh->b_uptodate = 0;
			bh->b_req = 0;
			return;
		}
	/* Uhhuh.. Nasty dead-lock possible here.. */
	if (bh->b_lock)
		return;
	/* Maybe the above fixes it, and maybe it doesn't boot. Life is interesting */
	lock_buffer(bh);
	if ((rw == WRITE && !bh->b_dirt) || (rw == READ && bh->b_uptodate)) {
		unlock_buffer(bh);
		return;
	}

/* we don't allow the write-requests to fill up the queue completely:
 * we want some room for reads: they take precedence. The last third
 * of the requests are only for reads.
 */
	max_req = (rw == READ) ? NR_REQUEST : ((NR_REQUEST*2)/3);

/* look for a free request. */
	local_irq_save(flags);

/* The scsi disk drivers and the IDE driver completely remove the request
 * from the queue when they start processing an entry.  For this reason
 * it is safe to continue to add links to the top entry for those devices.
 */
	if ((   major == IDE0_MAJOR	/* same as HD_MAJOR */
	     || major == IDE1_MAJOR
	     || major == FLOPPY_MAJOR
	     || major == SCSI_DISK_MAJOR
	     || major == SCSI_CDROM_MAJOR)
	    && (req = blk_dev[major].current_request))
	{
#ifdef CONFIG_BLK_DEV_HD
	        if (major == HD_MAJOR || major == FLOPPY_MAJOR)
#else
		if (major == FLOPPY_MAJOR)
#endif /* CONFIG_BLK_DEV_HD */
			req = req->next;
		while (req) {
			if (req->dev == bh->b_dev &&
			    req->cmd == rw &&
			    req->sector + req->nr_sectors == sector &&
			    req->nr_sectors < 244)
			{
				req->bhtail->b_reqnext = bh;
				req->bhtail = bh;
				req->nr_sectors += count;
				mark_buffer_clean(bh);

				local_irq_restore(flags);

				return;
			}

			if (req->dev == bh->b_dev &&
			    req->cmd == rw &&
			    req->sector - count == sector &&
			    req->nr_sectors < 244)
			{
			    	req->nr_sectors += count;
			    	bh->b_reqnext = req->bh;
			    	req->buffer = bh->b_data;
			    	req->current_nr_sectors = count;
			    	req->sector = sector;
					mark_buffer_clean(bh);
			    	req->bh = bh;

					local_irq_restore(flags);
			    	return;
			}    

			req = req->next;
		}
	}

/* find an unused request. */
	req = get_request(max_req, bh->b_dev);

	local_irq_restore(flags);


/* if no request available: if rw_ahead, forget it; otherwise try again blocking.. */
	if (!req) {
		if (rw_ahead) {
			unlock_buffer(bh);
			return;
		}
		req = __get_request_wait(max_req, bh->b_dev);
	}

/* fill up the request-info, and add it to the queue */
	req->cmd = rw;
	req->errors = 0;
	req->sector = sector;
	req->nr_sectors = count;
	req->current_nr_sectors = count;
	req->buffer = bh->b_data;
	req->bh = bh;
	req->bhtail = bh;
	req->next = NULL;
	add_request(major+blk_dev,req);
}

/* This function can be used to request a number of buffers from a block
   device. Currently the only restriction is that all buffers must belong to
   the same device */

void ll_rw_block(int rw, int nr, struct buffer_head * bh[])
{
	unsigned int major;
	struct request plug;
	int correct_size;
	struct blk_dev_struct * dev;
	int i;

	/* Make sure that the first block contains something reasonable */
	while (!*bh) {
		bh++;
		if (--nr <= 0)
			return;
	};

	dev = NULL;
	if ((major = MAJOR(bh[0]->b_dev)) < MAX_BLKDEV)
		dev = blk_dev + major;
	if (!dev || !dev->request_fn) {
		printk(
	"ll_rw_block: Trying to read nonexistent block-device %04lX (%ld)\n",
		       (unsigned long) bh[0]->b_dev, bh[0]->b_blocknr);
		goto sorry;
	}

	/* Determine correct block size for this device.  */
	correct_size = BLOCK_SIZE;
	if (blksize_size[major]) {
		i = blksize_size[major][MINOR(bh[0]->b_dev)];
		if (i)
			correct_size = i;
	}

	/* Verify requested block sizes.  */
	for (i = 0; i < nr; i++) {
		if (bh[i] && bh[i]->b_size != correct_size) {
			printk(
			"ll_rw_block: only %d-char blocks implemented (%lu)\n",
			       correct_size, bh[i]->b_size);
			goto sorry;
		}
	}

	if ((rw == WRITE || rw == WRITEA) && is_read_only(bh[0]->b_dev)) {
		printk("Can't write to read-only device 0x%X\n",bh[0]->b_dev);
		goto sorry;
	}

	/* If there are no pending requests for this device, then we insert
	   a dummy request for that device.  This will prevent the request
	   from starting until we have shoved all of the blocks into the
	   queue, and then we let it rip.  */

	if (nr > 1)
		plug_device(dev, &plug);
	for (i = 0; i < nr; i++) {
		if (bh[i]) {
			bh[i]->b_req = 1;
			make_request(major, rw, bh[i]);
			if (rw == READ || rw == READA)
				kstat.pgpgin++;
			else
				kstat.pgpgout++;
		}
	}
	unplug_device(dev);
	return;

      sorry:
	for (i = 0; i < nr; i++) {
		if (bh[i])
			bh[i]->b_dirt = bh[i]->b_uptodate = 0;
	}
	return;
}

static struct request all_requests[NR_REQUEST];
int ramdisk_size = 4 * 1024;

/* blk_dev_struct is:
 *	do_request-address
 *	next-request
 */
struct blk_dev_struct blk_dev[MAX_BLKDEV] = {
	{ NULL, NULL },		/* 0 no_dev */
	{ NULL, NULL },		/* 1 dev mem */
	{ NULL, NULL },		/* 2 dev fd */
	{ NULL, NULL },		/* 3 dev ide0 or hd */
	{ NULL, NULL },		/* 4 dev ttyx */
	{ NULL, NULL },		/* 5 dev tty */
	{ NULL, NULL },		/* 6 dev lp */
	{ NULL, NULL },		/* 7 dev pipes */
	{ NULL, NULL },		/* 8 dev sd */
	{ NULL, NULL },		/* 9 dev st */
	{ NULL, NULL },		/* 10 */
	{ NULL, NULL },		/* 11 */
	{ NULL, NULL },		/* 12 */
	{ NULL, NULL },		/* 13 */
	{ NULL, NULL },		/* 14 */
	{ NULL, NULL },		/* 15 */
	{ NULL, NULL },		/* 16 */
	{ NULL, NULL },		/* 17 */
	{ NULL, NULL },		/* 18 */
	{ NULL, NULL },		/* 19 */
	{ NULL, NULL },		/* 20 */
	{ NULL, NULL },		/* 21 */
	{ NULL, NULL },		/* 22 dev ide1 */
	{ NULL, NULL },		/* 23 */
	{ NULL, NULL },		/* 24 */
	{ NULL, NULL },		/* 25 */
	{ NULL, NULL },		/* 26 */
	{ NULL, NULL },		/* 27 */
	{ NULL, NULL },		/* 28 */
	{ NULL, NULL },		/* 29 */
	{ NULL, NULL },		/* 30 */
};

/*
 * blksize_size contains the size of all block-devices:
 *
 * blksize_size[MAJOR][MINOR]
 *
 * if (!blksize_size[MAJOR]) then 1024 bytes is assumed.
 */
int * blksize_size[MAX_BLKDEV] = { NULL, NULL, };

extern int *blk_size[];
extern int *blksize_size[];

#define MAX_BUF_PER_PAGE (PAGE_SIZE / 512)
#define NBUF 64

int block_write(struct inode * inode, struct file * filp, const char * buf, int count)
{
	int blocksize, blocksize_bits, i, j, buffercount,write_error;
	int block, blocks;
	loff_t offset;
	int chars;
	int written = 0;
	int cluster_list[MAX_BUF_PER_PAGE];
	struct buffer_head * bhlist[NBUF];
	int blocks_per_cluster;
	unsigned int size;
	unsigned int dev;
	struct buffer_head * bh, *bufferlist[NBUF];
	register char * p;
	int excess;

	write_error = buffercount = 0;
	dev = inode->i_rdev;
	if ( is_read_only( inode->i_rdev ))
		return -EPERM;
	blocksize = BLOCK_SIZE;
	if (blksize_size[MAJOR(dev)] && blksize_size[MAJOR(dev)][MINOR(dev)])
		blocksize = blksize_size[MAJOR(dev)][MINOR(dev)];

	i = blocksize;
	blocksize_bits = 0;
	while(i != 1) {
		blocksize_bits++;
		i >>= 1;
	}

	blocks_per_cluster = PAGE_SIZE / blocksize;

	block = filp->f_pos >> blocksize_bits;
	offset = filp->f_pos & (blocksize-1);

	if (blk_size[MAJOR(dev)])
		size = ((loff_t) blk_size[MAJOR(dev)][MINOR(dev)] << BLOCK_SIZE_BITS) >> blocksize_bits;
	else
		size = INT_MAX;
	while (count>0) {
		if (block >= size)
			return written;
		chars = blocksize - offset;
		if (chars > count)
			chars=count;

#if 0
		if (chars == blocksize)
			bh = getblk(dev, block, blocksize);
		else
			bh = breada(dev,block,block+1,block+2,-1);

#else
		for(i=0; i<blocks_per_cluster; i++) cluster_list[i] = block+i;
		if((block % blocks_per_cluster) == 0)
		  generate_cluster(dev, cluster_list, blocksize);
		bh = getblk(dev, block, blocksize);

		if (chars != blocksize && !bh->b_uptodate) {
		  if(!filp->f_reada ||
		     !read_ahead[MAJOR(dev)]) {
		    /* We do this to force the read of a single buffer */
		    brelse(bh);
		    bh = bread(dev,block,blocksize);
		  } else {
		    /* Read-ahead before write */
		    blocks = read_ahead[MAJOR(dev)] / (blocksize >> 9) / 2;
		    if (block + blocks > size) blocks = size - block;
		    if (blocks > NBUF) blocks=NBUF;
		    excess = (block + blocks) % blocks_per_cluster;
		    if ( blocks > excess )
			blocks -= excess;
		    bhlist[0] = bh;
		    for(i=1; i<blocks; i++){
		      if(((i+block) % blocks_per_cluster) == 0) {
			for(j=0; j<blocks_per_cluster; j++) cluster_list[j] = block+i+j;
			generate_cluster(dev, cluster_list, blocksize);
		      };
		      bhlist[i] = getblk (dev, block+i, blocksize);
		      if(!bhlist[i]){
			while(i >= 0) brelse(bhlist[i--]);
			return written? written: -EIO;
		      };
		    };
		    ll_rw_block(READ, blocks, bhlist);
		    for(i=1; i<blocks; i++) brelse(bhlist[i]);
		    wait_on_buffer(bh);
		      
		  };
		};
#endif
		block++;
		if (!bh)
			return written?written:-EIO;
		p = offset + bh->b_data;
		offset = 0;
		filp->f_pos += chars;
		written += chars;
		count -= chars;
		memcpy_fromfs(p,buf,chars);
		p += chars;
		buf += chars;
		bh->b_uptodate = 1;
		mark_buffer_dirty(bh, 0);
		if (filp->f_flags & O_SYNC)
			bufferlist[buffercount++] = bh;
		else
			brelse(bh);
		if (buffercount == NBUF){
			ll_rw_block(WRITE, buffercount, bufferlist);
			for(i=0; i<buffercount; i++){
				wait_on_buffer(bufferlist[i]);
				if (!bufferlist[i]->b_uptodate)
					write_error=1;
				brelse(bufferlist[i]);
			}
			buffercount=0;
		}
		if(write_error)
			break;
	}
	if ( buffercount ){
		ll_rw_block(WRITE, buffercount, bufferlist);
		for(i=0; i<buffercount; i++){
			wait_on_buffer(bufferlist[i]);
			if (!bufferlist[i]->b_uptodate)
				write_error=1;
			brelse(bufferlist[i]);
		}
	}		
	filp->f_reada = 1;
	if(write_error)
		return -EIO;
	return written;
}

int block_read(struct inode * inode, struct file * filp, char * buf, int count)
{
	unsigned int block;
	loff_t offset;
	int blocksize;
	int blocksize_bits, i;
	unsigned int blocks, rblocks, left;
	int bhrequest, uptodate;
	int cluster_list[MAX_BUF_PER_PAGE];
	int blocks_per_cluster;
	struct buffer_head ** bhb, ** bhe;
	struct buffer_head * buflist[NBUF];
	struct buffer_head * bhreq[NBUF];
	unsigned int chars;
	loff_t size;
	unsigned int dev;
	int read;
	int excess;

	dev = inode->i_rdev;
	blocksize = BLOCK_SIZE;
	if (blksize_size[MAJOR(dev)] && blksize_size[MAJOR(dev)][MINOR(dev)])
		blocksize = blksize_size[MAJOR(dev)][MINOR(dev)];
	i = blocksize;
	blocksize_bits = 0;
	while (i != 1) {
		blocksize_bits++;
		i >>= 1;
	}

	offset = filp->f_pos;
	if (blk_size[MAJOR(dev)])
		size = (loff_t) blk_size[MAJOR(dev)][MINOR(dev)] << BLOCK_SIZE_BITS;
	else
		size = INT_MAX;

	blocks_per_cluster = PAGE_SIZE / blocksize;

	if (offset > size)
		left = 0;
	/* size - offset might not fit into left, so check explicitly. */
	else if (size - offset > INT_MAX)
		left = INT_MAX;
	else
		left = size - offset;
	if (left > count)
		left = count;
	if (left <= 0)
		return 0;
	read = 0;
	block = offset >> blocksize_bits;
	offset &= blocksize-1;
	size >>= blocksize_bits;
	rblocks = blocks = (left + offset + blocksize - 1) >> blocksize_bits;
	bhb = bhe = buflist;
	if (filp->f_reada) {
	        if (blocks < read_ahead[MAJOR(dev)] / (blocksize >> 9))
			blocks = read_ahead[MAJOR(dev)] / (blocksize >> 9);
		excess = (block + blocks) % blocks_per_cluster;
		if ( blocks > excess )
			blocks -= excess;		
		if (rblocks > blocks)
			blocks = rblocks;
		
	}
	if (block + blocks > size)
		blocks = size - block;

	/* We do this in a two stage process.  We first try and request
	   as many blocks as we can, then we wait for the first one to
	   complete, and then we try and wrap up as many as are actually
	   done.  This routine is rather generic, in that it can be used
	   in a filesystem by substituting the appropriate function in
	   for getblk.

	   This routine is optimized to make maximum use of the various
	   buffers and caches. */

	do {
		bhrequest = 0;
		uptodate = 1;
		while (blocks) {
			--blocks;
#if 1
			if((block % blocks_per_cluster) == 0) {
			  for(i=0; i<blocks_per_cluster; i++) cluster_list[i] = block+i;
			  generate_cluster(dev, cluster_list, blocksize);
			}
#endif
			*bhb = getblk(dev, block++, blocksize);
			if (*bhb && !(*bhb)->b_uptodate) {
				uptodate = 0;
				bhreq[bhrequest++] = *bhb;
			}

			if (++bhb == &buflist[NBUF])
				bhb = buflist;

			/* If the block we have on hand is uptodate, go ahead
			   and complete processing. */
			if (uptodate)
				break;
			if (bhb == bhe)
				break;
		}

		/* Now request them all */
		if (bhrequest) {
			ll_rw_block(READ, bhrequest, bhreq);
			refill_freelist(blocksize);
		}

		do { /* Finish off all I/O that has actually completed */
			if (*bhe) {
				wait_on_buffer(*bhe);
				if (!(*bhe)->b_uptodate) {	/* read error? */
				        brelse(*bhe);
					if (++bhe == &buflist[NBUF])
					  bhe = buflist;
					left = 0;
					break;
				}
			}			
			if (left < blocksize - offset)
				chars = left;
			else
				chars = blocksize - offset;
			filp->f_pos += chars;
			left -= chars;
			read += chars;
			if (*bhe) {
				memcpy_tofs(buf,offset+(*bhe)->b_data,chars);
				brelse(*bhe);
				buf += chars;
			} else {
				while (chars-->0)
					put_fs_byte(0,buf++);
			}
			offset = 0;
			if (++bhe == &buflist[NBUF])
				bhe = buflist;
		} while (left > 0 && bhe != bhb && (!*bhe || !(*bhe)->b_lock));
	} while (left > 0);

/* Release the read-ahead blocks */
	while (bhe != bhb) {
		brelse(*bhe);
		if (++bhe == &buflist[NBUF])
			bhe = buflist;
	};
	if (!read)
		return -EIO;
	filp->f_reada = 1;
	return read;
}

int block_fsync(struct inode *inode, struct file *filp)
{
	return fsync_dev (inode->i_rdev);
}

extern void rd_init(int length);

void blk_dev_init(void)
{
	struct request * req;

	req = all_requests + NR_REQUEST;
	while (--req >= all_requests) {
		req->dev = -1;
		req->next = NULL;
	}
	if (ramdisk_size)
		rd_init(ramdisk_size*1024);
}

