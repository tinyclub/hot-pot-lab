/**********************************************************
 * Filename	: buffer.c
 * Comment	: 文件系统缓冲区管理模块
 * Developer: 谢宝友
 * Date		: 2014-09-23
 **********************************************************/
#include <base/string.h>
#include <dim-sum/timer.h>
#include <kapi/dim-sum/task.h>
#include <linux/types.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <asm/page.h>
#include <dim-sum/mem.h>
#include <dim-sum/errno.h>
#include <dim-sum/boot_allotter.h>
#include <dim-sum/sched.h>
#include <dim-sum/printk.h>
#include <asm/current.h>
#include <fs/locks.h>


#define MAP_NR(addr)		(((unsigned long)(addr)) >> PAGE_SHIFT)

#define NR_SIZES 4
static char buffersize_index[9] = {-1,  0,  1, -1,  2, -1, -1, -1, 3};
static short int bufferindex_size[NR_SIZES] = {512, 1024, 2048, 4096};

#define BUFSIZE_INDEX(X) ((int) buffersize_index[(X)>>9])
#define MAX_BUF_PER_PAGE (PAGE_SIZE / 512)

static int grow_buffers(int pri, int size);
static int shrink_specific_buffers(unsigned int priority, int size);
static int maybe_shrink_lav_buffers(int);
static struct buffer_head * find_buffer(dev_t dev, int block, int size);
static inline void remove_from_lru_list(struct buffer_head * bh);


#define N_PARAM 9
#define LAV

#define _hashfn(dev,block) (((unsigned)(dev^block))%nr_hash)
#define hash(dev,block) hash_table[_hashfn(dev,block)]


/* Here is the parameter block for the bdflush process. */
static void wakeup_bdflush(int);

static union bdflush_param{
	struct {
		int nfract;  /* Percentage of buffer cache dirty to 
				activate bdflush */
		int ndirty;  /* Maximum number of dirty blocks to write out per
				wake-cycle */
		int nrefill; /* Number of clean buffers to try and obtain
				each time we call refill */
		int nref_dirt; /* Dirty buffer threshold for activating bdflush
				  when trying to refill buffers. */
		int clu_nfract;  /* Percentage of buffer cache to scan to 
				    search for free clusters */
		int age_buffer;  /* Time for normal buffer to age before 
				    we flush it */
		int age_super;  /* Time for superblock to age before we 
				   flush it */
		int lav_const;  /* Constant used for load average (time
				   constant */
		int lav_ratio;  /* Used to determine how low a lav for a
				   particular size can go before we start to
				   trim back the buffers */
	} b_un;
	unsigned int data[N_PARAM];
} bdf_prm = {{25, 500, 64, 256, 15, 3000, 500, 1884, 2}};


static int nr_hash = 0;  /* Size of hash table */
static struct buffer_head ** hash_table;
///struct buffer_head ** buffer_pages;
static struct buffer_head * lru_list[NR_LIST] = {NULL, };
static struct buffer_head * free_list[NR_SIZES] = {NULL, };
static struct buffer_head * unused_list = NULL;
static DECLARE_WAIT_QUEUE_HEAD(buffer_wait);

int nr_buffers = 0;
int nr_buffers_type[NR_LIST] = {0,};
int nr_buffers_size[NR_SIZES] = {0,};
int nr_buffers_st[NR_SIZES][NR_LIST] = {{0,},};
int buffer_usage[NR_SIZES] = {0,};  /* Usage counts used to determine load average */
int buffers_lav[NR_SIZES] = {0,};  /* Load average of buffer usage */
int nr_free[NR_SIZES] = {0,};
int buffermem = 0;
int nr_buffer_heads = 0;
extern int *blksize_size[];


/*
 * Why like this, I hear you say... The reason is race-conditions.
 * As we don't lock buffers (unless we are reading them, that is),
 * something might happen to it while we sleep (ie a read-error
 * will force it bad). This shouldn't really happen currently, but
 * the code is ready.
 */
struct buffer_head * get_hash_table(dev_t dev, int block, int size)
{
	struct buffer_head * bh;

	for (;;) {
		if (!(bh=find_buffer(dev,block,size)))
			return NULL;
		bh->b_count++;
		wait_on_buffer(bh);
		if (bh->b_dev == dev && bh->b_blocknr == block && bh->b_size == size)
			return bh;
		bh->b_count--;
	}
}


static inline void put_last_lru(struct buffer_head * bh)
{
	if (!bh)
		return;
	if (bh == lru_list[bh->b_list]) {
		lru_list[bh->b_list] = bh->b_next_free;
		return;
	}
	if(bh->b_dev == 0xffff) panic("Wrong block for lru list");
	remove_from_lru_list(bh);
/* add to back of free list */

	if(!lru_list[bh->b_list]) {
		lru_list[bh->b_list] = bh;
		lru_list[bh->b_list]->b_prev_free = bh;
	};

	bh->b_next_free = lru_list[bh->b_list];
	bh->b_prev_free = lru_list[bh->b_list]->b_prev_free;
	lru_list[bh->b_list]->b_prev_free->b_next_free = bh;
	lru_list[bh->b_list]->b_prev_free = bh;
}

static inline void put_last_free(struct buffer_head * bh)
{
        int isize;
	if (!bh)
		return;

	isize = BUFSIZE_INDEX(bh->b_size);	
	bh->b_dev = 0xffff;  /* So it is obvious we are on the free list */
/* add to back of free list */

	if(!free_list[isize]) {
		free_list[isize] = bh;
		bh->b_prev_free = bh;
	};

	nr_free[isize]++;
	bh->b_next_free = free_list[isize];
	bh->b_prev_free = free_list[isize]->b_prev_free;
	free_list[isize]->b_prev_free->b_next_free = bh;
	free_list[isize]->b_prev_free = bh;
}

static inline void insert_into_queues(struct buffer_head * bh)
{
	/* put at end of free list */
    if(bh->b_dev == 0xffff) {
		put_last_free(bh);
		return;
	};
	//printk("xby_debug in insert_into_queues, step 2\n");
	if(!lru_list[bh->b_list]) {
		lru_list[bh->b_list] = bh;
		bh->b_prev_free = bh;
	};
	//printk("xby_debug in insert_into_queues, step 3\n");
	if (bh->b_next_free) panic("VFS: buffer LRU pointers corrupted");
	//printk("xby_debug in insert_into_queues, step 3.1\n");
	bh->b_next_free = lru_list[bh->b_list];
	//printk("xby_debug in insert_into_queues, step 3.2, %p\n", lru_list[bh->b_list]);
	bh->b_prev_free = lru_list[bh->b_list]->b_prev_free;
	//printk("xby_debug in insert_into_queues, step 3.3\n");
	lru_list[bh->b_list]->b_prev_free->b_next_free = bh;
	//printk("xby_debug in insert_into_queues, step 3.4\n");
	lru_list[bh->b_list]->b_prev_free = bh;
	nr_buffers_type[bh->b_list]++;
	//printk("xby_debug in insert_into_queues, step 3.5\n");
	nr_buffers_st[BUFSIZE_INDEX(bh->b_size)][bh->b_list]++;
	//printk("xby_debug in insert_into_queues, step 4\n");
/* put the buffer in new hash-queue if it has a device */
	bh->b_prev = NULL;
	bh->b_next = NULL;
	//printk("xby_debug in insert_into_queues, step 5\n");
	if (!bh->b_dev)
		return;
	//printk("xby_debug in insert_into_queues, step 6\n");
	bh->b_next = hash(bh->b_dev,bh->b_blocknr);
	hash(bh->b_dev,bh->b_blocknr) = bh;
	if (bh->b_next)
		bh->b_next->b_prev = bh;
	//printk("xby_debug in insert_into_queues, step 7\n");
}

static inline void remove_from_hash_queue(struct buffer_head * bh)
{
	if (bh->b_next)
		bh->b_next->b_prev = bh->b_prev;
	if (bh->b_prev)
		bh->b_prev->b_next = bh->b_next;
	if (hash(bh->b_dev,bh->b_blocknr) == bh)
		hash(bh->b_dev,bh->b_blocknr) = bh->b_next;
	bh->b_next = bh->b_prev = NULL;
}

static inline void remove_from_lru_list(struct buffer_head * bh)
{
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("VFS: LRU block list corrupted");
	if (bh->b_dev == 0xffff) panic("LRU list corrupted");
	bh->b_prev_free->b_next_free = bh->b_next_free;
	bh->b_next_free->b_prev_free = bh->b_prev_free;

	if (lru_list[bh->b_list] == bh)
		 lru_list[bh->b_list] = bh->b_next_free;
	if(lru_list[bh->b_list] == bh)
		 lru_list[bh->b_list] = NULL;
	bh->b_next_free = bh->b_prev_free = NULL;
}

static inline void remove_from_free_list(struct buffer_head * bh)
{
        int isize = BUFSIZE_INDEX(bh->b_size);
	if (!(bh->b_prev_free) || !(bh->b_next_free))
		panic("VFS: Free block list corrupted");
	if(bh->b_dev != 0xffff) panic("Free list corrupted");
	if(!free_list[isize])
		 panic("Free list empty");
	nr_free[isize]--;
	if(bh->b_next_free == bh)
		 free_list[isize] = NULL;
	else {
		bh->b_prev_free->b_next_free = bh->b_next_free;
		bh->b_next_free->b_prev_free = bh->b_prev_free;
		if (free_list[isize] == bh)
			 free_list[isize] = bh->b_next_free;
	};
	bh->b_next_free = bh->b_prev_free = NULL;
}

static inline void remove_from_queues(struct buffer_head * bh)
{
        if(bh->b_dev == 0xffff) {
		remove_from_free_list(bh); /* Free list entries should not be
					      in the hash queue */
		return;
	};
	nr_buffers_type[bh->b_list]--;
	nr_buffers_st[BUFSIZE_INDEX(bh->b_size)][bh->b_list]--;
	remove_from_hash_queue(bh);
	remove_from_lru_list(bh);
}

void refile_buffer(struct buffer_head * buf){
	int dispose;
	if(buf->b_dev == 0xffff) panic("Attempt to refile free buffer\n");
	if (buf->b_dirt)
		dispose = BUF_DIRTY;
#ifdef xby_debug
	else if (mem_map[MAP_NR((unsigned long) buf->b_data)] > 1)
		dispose = BUF_SHARED;
#else
#endif
	else if (buf->b_lock)
		dispose = BUF_LOCKED;
	else if (buf->b_list == BUF_SHARED)
		dispose = BUF_UNSHARED;
	else
		dispose = BUF_CLEAN;
	if(dispose == BUF_CLEAN) buf->b_lru_time = FS_CURRENT_TIME;
	if(dispose != buf->b_list)  {
		if(dispose == BUF_DIRTY || dispose == BUF_UNSHARED)
			 buf->b_lru_time = FS_CURRENT_TIME;
		if(dispose == BUF_LOCKED && 
		   (buf->b_flushtime - buf->b_lru_time) <= bdf_prm.b_un.age_super)
			 dispose = BUF_LOCKED1;
		remove_from_queues(buf);
		buf->b_list = dispose;
		insert_into_queues(buf);
		if(dispose == BUF_DIRTY && nr_buffers_type[BUF_DIRTY] > 
		   (nr_buffers - nr_buffers_type[BUF_SHARED]) *
		   bdf_prm.b_un.nfract/100)
			 wakeup_bdflush(0);
	}
}

/*
 * bread() reads a specified block and returns the buffer that contains
 * it. It returns NULL if the block was unreadable.
 */
struct buffer_head * bread(dev_t dev, int block, int size)
{
	struct buffer_head * bh;

	if (!(bh = getblk(dev, block, size))) {
		printk("VFS: bread: READ error on device %d/%d\n",
						MAJOR(dev), MINOR(dev));
		return NULL;
	}
	if (bh->b_uptodate)
		return bh;
	ll_rw_block(READ, 1, &bh);
	wait_on_buffer(bh);
	if (bh->b_uptodate)
		return bh;
	brelse(bh);
	return NULL;
}


static struct buffer_head * find_buffer(dev_t dev, int block, int size)
{		
	struct buffer_head * tmp;

	for (tmp = hash(dev,block) ; tmp != NULL ; tmp = tmp->b_next)
	{
		if (tmp->b_dev==dev && tmp->b_blocknr==block)
		{
			if (tmp->b_size == size)
			{
				return tmp;
			}
			else 
			{
				printk("VFS: Wrong blocksize on device %d/%d\n",
							MAJOR(dev), MINOR(dev));
				return NULL;
			}
		}
	}
	return NULL;
}


/*
 * See fs/inode.c for the weird use of volatile..
 */
static void put_unused_buffer_head(struct buffer_head * bh)
{
	//struct wait_queue * wait;

	//wait = ((volatile struct buffer_head *) bh)->b_wait;
	memset(bh,0,sizeof(*bh));
	//((volatile struct buffer_head *) bh)->b_wait = wait;
	init_waitqueue_head(&bh->b_wait);
	bh->b_next_free = unused_list;
	unused_list = bh;
}


#define BUFFER_PAGE_COUNT 512
static LIST_HEAD(list_buffer_page_free);
static LIST_HEAD(list_buffer_page_used);


struct buffer_page_desc{
	struct list_head list;
	unsigned long page;
};
unsigned long get_free_buffer_page(int pri)
{
	struct list_head *list;
	struct buffer_page_desc *desc;
	if (list_empty(&list_buffer_page_free))
		return 0;

	list = list_buffer_page_free.next;
	desc = list_entry(list, struct buffer_page_desc, list);
	list_del_init(list);

	list_add(&desc->list, &list_buffer_page_used);
	return desc->page;
}

void free_buffer_page(unsigned long page)
{
	struct list_head *list;
	struct buffer_page_desc *desc = NULL;
	list_for_each(list, &list_buffer_page_used)
	{
		desc = list_entry(list, struct buffer_page_desc, list);
		if (desc->page == page)
		{
			break;
		}
	}

	if (desc)
	{
		list_del_init(list);
		list_add(&desc->list, &list_buffer_page_free);
	}
}




void init_buffer_page(void)
{
	int i;
	struct buffer_page_desc *desc;
	for (i = 0; i < BUFFER_PAGE_COUNT; i++)
	{
		desc = dim_sum_mem_alloc(sizeof(struct buffer_page_desc), MEM_NORMAL);
		INIT_LIST_HEAD(&desc->list);
		desc->page = (unsigned long)dim_sum_pages_alloc(0, MEM_NORMAL);
		list_add_tail(&desc->list, &list_buffer_page_free);
	}
}

static void get_more_buffer_heads(void)
{
	int i;
	struct buffer_head * bh;

	if (unused_list)
		return;

	if (!(bh = (struct buffer_head*) get_free_buffer_page(GFP_BUFFER)))
		return;

	for (nr_buffer_heads+=i=PAGE_SIZE/sizeof*bh ; i>0; i--) {
		bh->b_next_free = unused_list;	/* only make link */
		unused_list = bh++;
	}
}

static struct buffer_head * get_unused_buffer_head(void)
{
	struct buffer_head * bh;

	get_more_buffer_heads();
	if (!unused_list)
		return NULL;
	bh = unused_list;
	unused_list = bh->b_next_free;
	bh->b_next_free = NULL;
	bh->b_data = NULL;
	bh->b_size = 0;
	bh->b_req = 0;
	init_waitqueue_head(&bh->b_wait);
	return bh;
}

/*
 * Create the appropriate buffers when given a page for data area and
 * the size of each buffer.. Use the bh->b_this_page linked list to
 * follow the buffers created.  Return NULL if unable to create more
 * buffers.
 */
static struct buffer_head * create_buffers(unsigned long page, unsigned long size)
{
	struct buffer_head *bh, *head;
	unsigned long offset;

	head = NULL;
	offset = PAGE_SIZE;
	while ((offset -= size) < PAGE_SIZE) {
		bh = get_unused_buffer_head();
		if (!bh)
			goto no_grow;
		bh->b_this_page = head;
		head = bh;
		bh->b_data = (char *) (page+offset);
		bh->b_size = size;
		bh->b_dev = 0xffff;  /* Flag as unused */
	}
	return head;
/*
 * In case anything failed, we just free everything we got.
 */
no_grow:
	bh = head;
	while (bh) {
		head = bh;
		bh = bh->b_this_page;
		put_unused_buffer_head(head);
	}
	return NULL;
}

/* This function tries to generate a new cluster of buffers
 * from a new page in memory.  We should only do this if we have
 * not expanded the buffer cache to the maximum size that we allow.
 */
static unsigned long try_to_generate_cluster(dev_t dev, int block, int size)
{
	struct buffer_head * bh, * tmp, * arr[MAX_BUF_PER_PAGE];
	int isize = BUFSIZE_INDEX(size);
	unsigned long offset;
	unsigned long page;
	int nblock;

	page = (unsigned long)get_free_buffer_page(GFP_NOBUFFER);
	if(!page) return 0;

	bh = create_buffers(page, size);
	if (!bh) {
		free_buffer_page(page);
		return 0;
	};
	nblock = block;
	for (offset = 0 ; offset < PAGE_SIZE ; offset += size) {
		if (find_buffer(dev, nblock++, size))
			 goto not_aligned;
	}
	tmp = bh;
	nblock = 0;
	while (1) {
		arr[nblock++] = bh;
		bh->b_count = 1;
		bh->b_dirt = 0;
		bh->b_flushtime = 0;
		bh->b_lock = 0;
		bh->b_uptodate = 0;
		bh->b_req = 0;
		bh->b_dev = dev;
		bh->b_list = BUF_CLEAN;
		bh->b_blocknr = block++;
		nr_buffers++;
		nr_buffers_size[isize]++;
		insert_into_queues(bh);
		if (bh->b_this_page)
			bh = bh->b_this_page;
		else
			break;
	}
	buffermem += PAGE_SIZE;
	//buffer_pages[MAP_NR(page)] = bh;
	bh->b_this_page = tmp;
	while (nblock-- > 0)
		brelse(arr[nblock]);
	return 4; /* ?? */
not_aligned:
	while ((tmp = bh) != NULL) {
		bh = bh->b_this_page;
		put_unused_buffer_head(tmp);
	}
	free_buffer_page(page);
	return 0;
}



/* ====================== Cluster patches for ext2 ==================== */

/*
 * try_to_reassign() checks if all the buffers on this particular page
 * are unused, and reassign to a new cluster them if this is true.
 */
static inline int try_to_reassign(struct buffer_head * bh, struct buffer_head ** bhp,
			   dev_t dev, unsigned int starting_block)
{
	unsigned long page;
	struct buffer_head * tmp, * p;

	*bhp = bh;
	page = (unsigned long) bh->b_data;
	page &= PAGE_MASK;
#ifdef xby_debug
	if(mem_map[MAP_NR(page)] != 1) return 0;
#endif

	tmp = bh;
	do {
		if (!tmp)
			 return 0;
		
		if (tmp->b_count || tmp->b_dirt || tmp->b_lock)
			 return 0;
		tmp = tmp->b_this_page;
	} while (tmp != bh);
	tmp = bh;
	
	while((unsigned long) tmp->b_data & (PAGE_SIZE - 1)) 
		 tmp = tmp->b_this_page;
	
	/* This is the buffer at the head of the page */
	bh = tmp;
	do {
		p = tmp;
		tmp = tmp->b_this_page;
		remove_from_queues(p);
		p->b_dev=dev;
		p->b_uptodate = 0;
		p->b_req = 0;
		p->b_blocknr=starting_block++;
		insert_into_queues(p);
	} while (tmp != bh);
	return 1;
}



/*
 * Try to find a free cluster by locating a page where
 * all of the buffers are unused.  We would like this function
 * to be atomic, so we do not call anything that might cause
 * the process to sleep.  The priority is somewhat similar to
 * the priority used in shrink_buffers.
 * 
 * My thinking is that the kernel should end up using whole
 * pages for the buffer cache as much of the time as possible.
 * This way the other buffers on a particular page are likely
 * to be very near each other on the free list, and we will not
 * be expiring data prematurely.  For now we only cannibalize buffers
 * of the same size to keep the code simpler.
 */
static __maybe_unused int reassign_cluster(dev_t dev, 
		     unsigned int starting_block, int size)
{
	struct buffer_head *bh;
        int isize = BUFSIZE_INDEX(size);
	int i;

	/* We want to give ourselves a really good shot at generating
	   a cluster, and since we only take buffers from the free
	   list, we "overfill" it a little. */

	while(nr_free[isize] < 32) refill_freelist(size);

	bh = free_list[isize];
	if(bh)
		 for (i=0 ; !i || bh != free_list[isize] ; bh = bh->b_next_free, i++) {
			 if (!bh->b_this_page)	continue;
			 if (try_to_reassign(bh, &bh, dev, starting_block))
				 return 4;
		 }
	return 0;
}

unsigned long generate_cluster(dev_t dev, int b[], int size)
{
	int i, offset;
	
	for (i = 0, offset = 0 ; offset < PAGE_SIZE ; i++, offset += size) {
		if(i && b[i]-1 != b[i-1]) return 0;  /* No need to cluster */
		if(find_buffer(dev, b[i], size)) return 0;
	};

	/* OK, we have a candidate for a new cluster */
	
	/* See if one size of buffer is over-represented in the buffer cache,
	   if so reduce the numbers of buffers */
	if(maybe_shrink_lav_buffers(size))
	 {
		 int retval;
		 retval = try_to_generate_cluster(dev, b[0], size);
		 if(retval) return retval;
	 };

#ifdef xby_debug
	if (nr_free_pages > min_free_pages*2) 
		 return try_to_generate_cluster(dev, b[0], size);
	else
		 return reassign_cluster(dev, b[0], size);
#else
	try_to_generate_cluster(dev, b[0], size);
	//reassign_cluster(dev, b[0], size);
	return 0;
#endif
}



/*
 * Ok, this is getblk, and it isn't very clear, again to hinder
 * race-conditions. Most of the code is seldom used, (ie repeating),
 * so it should be much more efficient than it looks.
 *
 * The algorithm is changed: hopefully better, and an elusive bug removed.
 *
 * 14.02.92: changed it to sync dirty buffers a bit: better performance
 * when the filesystem starts to get full of dirty blocks (I hope).
 */
struct buffer_head * getblk(dev_t dev, int block, int size)
{
	struct buffer_head * bh;
        int isize = BUFSIZE_INDEX(size);

	/* Update this for the buffer size lav. */
	buffer_usage[isize]++;

	//printk("xby_debug in getblk, step 1\n");
	/* If there are too many dirty buffers, we wake up the update process
	   now so as to ensure that there are still clean buffers available
	   for user processes to use (and dirty) */
repeat:
	bh = get_hash_table(dev, block, size);
	if (bh) {
		if (bh->b_uptodate && !bh->b_dirt)
			 put_last_lru(bh);
		if(!bh->b_dirt) bh->b_flushtime = 0;
		bh->b_reqnext = NULL;
		return bh;
	}
	//printk("xby_debug in getblk, step 2\n");

	while(!free_list[isize]) refill_freelist(size);
	if (find_buffer(dev,block,size))
		 goto repeat;
	//printk("xby_debug in getblk, step 3\n");

	bh = free_list[isize];
	remove_from_free_list(bh);

/* OK, FINALLY we know that this buffer is the only one of its kind, */
/* and that it's unused (b_count=0), unlocked (b_lock=0), and clean */
	bh->b_count=1;
	bh->b_dirt=0;
	bh->b_lock=0;
	bh->b_uptodate=0;
	bh->b_flushtime = 0;
	bh->b_req=0;
	bh->b_dev=dev;
	bh->b_blocknr=block;
	bh->b_reqnext = NULL;
	//printk("xby_debug in getblk, step 4\n");
	insert_into_queues(bh);
	//printk("xby_debug in getblk, step 5\n");
	return bh;
}

void brelse(struct buffer_head * buf)
{
	if (!buf)
		return;
	wait_on_buffer(buf);

	/* If dirty, mark the time this buffer should be written back */
	set_writetime(buf, 0);
	refile_buffer(buf);

	if (buf->b_count) {
		if (--buf->b_count)
			return;
		wake_up(&buffer_wait);
		return;
	}
	printk("VFS: brelse: Trying to free free buffer\n");
}

void set_writetime(struct buffer_head * buf, int flag)
{
        int newtime;

	if (buf->b_dirt){
		/* Move buffer to dirty list if jiffies is clear */
		newtime = FS_CURRENT_TIME + (flag ? bdf_prm.b_un.age_super : 
				     bdf_prm.b_un.age_buffer);
		if(!buf->b_flushtime || buf->b_flushtime > newtime)
			 buf->b_flushtime = newtime;
	} else {
		buf->b_flushtime = 0;
	}
}


#define BADNESS(bh) (((bh)->b_dirt<<1)+(bh)->b_lock)

extern int ker_Sleep(int iMiniSec);

void refill_freelist(int size)
{
	struct buffer_head * bh, * tmp;
	struct buffer_head * candidate[NR_LIST];
	unsigned int best_time, winner;
        int isize = BUFSIZE_INDEX(size);
	int buffers[NR_LIST];
	int i;
	int needed;

	/* First see if we even need this.  Sometimes it is advantageous
	 to request some blocks in a filesystem that we know that we will
	 be needing ahead of time. */

	if (nr_free[isize] > 100)
		return;

	/* If there are too many dirty buffers, we wake up the update process
	   now so as to ensure that there are still clean buffers available
	   for user processes to use (and dirty) */
	
	/* We are going to try and locate this much memory */
	needed =bdf_prm.b_un.nrefill * size;  

	while (needed > 0 && grow_buffers(GFP_BUFFER, size)) {
		needed -= PAGE_SIZE;
	}

	if(needed <= 0) return;

	/* See if there are too many buffers of a different size.
	   If so, victimize them */

	while(maybe_shrink_lav_buffers(size))
	 {
		 if(!grow_buffers(GFP_BUFFER, size)) break;
		 needed -= PAGE_SIZE;
		 if(needed <= 0) return;
	 };

	/* OK, we cannot grow the buffer cache, now try and get some
	   from the lru list */

	/* First set the candidate pointers to usable buffers.  This
	   should be quick nearly all of the time. */

repeat0:
	for(i=0; i<NR_LIST; i++){
		if(i == BUF_DIRTY || i == BUF_SHARED || 
		   nr_buffers_type[i] == 0) {
			candidate[i] = NULL;
			buffers[i] = 0;
			continue;
		}
		buffers[i] = nr_buffers_type[i];
		for (bh = lru_list[i]; buffers[i] > 0; bh = tmp, buffers[i]--)
		 {
			 if(buffers[i] < 0) panic("Here is the problem");
			 tmp = bh->b_next_free;
			 if (!bh) break;

#ifdef xby_debug
			 if (mem_map[MAP_NR((unsigned long) bh->b_data)] != 1 ||
			     bh->b_dirt) {
				 refile_buffer(bh);
				 continue;
			 };
#endif
			 if (bh->b_count || bh->b_size != size)
				  continue;
			 
			 /* Buffers are written in the order they are placed 
			    on the locked list. If we encounter a locked
			    buffer here, this means that the rest of them
			    are also locked */
			 if(bh->b_lock && (i == BUF_LOCKED || i == BUF_LOCKED1)) {
				 buffers[i] = 0;
				 break;
			 }
			 
			 if (BADNESS(bh)) continue;
			 break;
		 };
		if(!buffers[i]) candidate[i] = NULL; /* Nothing on this list */
		else candidate[i] = bh;
		if(candidate[i] && candidate[i]->b_count) panic("Here is the problem");
	}
	
 repeat:
	if(needed <= 0) return;
	
	/* Now see which candidate wins the election */
	
	winner = best_time = UINT_MAX;	
	for(i=0; i<NR_LIST; i++){
		if(!candidate[i]) continue;
		if(candidate[i]->b_lru_time < best_time){
			best_time = candidate[i]->b_lru_time;
			winner = i;
		}
	}
	
	/* If we have a winner, use it, and then get a new candidate from that list */
	if(winner != UINT_MAX) {
		i = winner;
		bh = candidate[i];
		candidate[i] = bh->b_next_free;
		if(candidate[i] == bh) candidate[i] = NULL;  /* Got last one */
		if (bh->b_count || bh->b_size != size)
			 panic("Busy buffer in candidate list\n");
#ifdef xby_debug
		if (mem_map[MAP_NR((unsigned long) bh->b_data)] != 1)
			 panic("Shared buffer in candidate list\n");
#endif	
		if (BADNESS(bh)) panic("Buffer in candidate list with BADNESS != 0\n");
	
		if(bh->b_dev == 0xffff) panic("Wrong list");
		remove_from_queues(bh);
		bh->b_dev = 0xffff;
		put_last_free(bh);
		needed -= bh->b_size;
		buffers[i]--;
		if(buffers[i] < 0) panic("Here is the problem");
		
		if(buffers[i] == 0) candidate[i] = NULL;
		
		/* Now all we need to do is advance the candidate pointer
		   from the winner list to the next usable buffer */
		if(candidate[i] && buffers[i] > 0){
			if(buffers[i] <= 0) panic("Here is another problem");
			for (bh = candidate[i]; buffers[i] > 0; bh = tmp, buffers[i]--) {
				if(buffers[i] < 0) panic("Here is the problem");
				tmp = bh->b_next_free;
				if (!bh) break;
#ifdef xby_debug				
				if (mem_map[MAP_NR((unsigned long) bh->b_data)] != 1 ||
				    bh->b_dirt) {
					refile_buffer(bh);
					continue;
				};
#endif
				
				if (bh->b_count || bh->b_size != size)
					 continue;
				
				/* Buffers are written in the order they are
				   placed on the locked list.  If we encounter
				   a locked buffer here, this means that the
				   rest of them are also locked */
				if(bh->b_lock && (i == BUF_LOCKED || i == BUF_LOCKED1)) {
					buffers[i] = 0;
					break;
				}
	      
				if (BADNESS(bh)) continue;
				break;
			};
			if(!buffers[i]) candidate[i] = NULL; /* Nothing here */
			else candidate[i] = bh;
			if(candidate[i] && candidate[i]->b_count) 
				 panic("Here is the problem");
		}
		
		goto repeat;
	}
	
	if(needed <= 0) return;

	/* Too bad, that was not enough. Try a little harder to grow some. */
	if (grow_buffers(GFP_BUFFER, size)) {
		needed -= PAGE_SIZE;
		goto repeat0;
	}
	else
	{
		wakeup_bdflush(1);
		needed -= PAGE_SIZE;
		ker_Sleep(1);
		goto repeat0;
	}
	
}


/*
 * Rewrote the wait-routines to use the "new" wait-queue functionality,
 * and getting rid of the cli-sti pairs. The wait-queue routines still
 * need cli-sti, but now it's just a couple of 386 instructions or so.
 *
 * Note that the real wait_on_buffer() is an inline function that checks
 * if 'b_wait' is set before calling this, so that the queues aren't set
 * up unnecessarily.
 */
void __wait_on_buffer(struct buffer_head * bh)
{
	wait_queue_t wait;
	init_waitqueue_entry (&wait, current);
		

	bh->b_count++;
	set_current_state (TASK_INTERRUPTIBLE);
	add_wait_queue (&bh->b_wait, &wait);
repeat:
	current->state = TASK_UNINTERRUPTIBLE;
	if (bh->b_lock) {
		TaskSwitch();
		goto repeat;
	}
	remove_wait_queue(&bh->b_wait, &wait);
	bh->b_count--;
	current->state = TASK_RUNNING;
}

/* Call sync_buffers with wait!=0 to ensure that the call does not
   return until all buffer writes have completed.  Sync() may return
   before the writes have finished; fsync() may not. */


/* Godamity-damn.  Some buffers (bitmaps for filesystems)
   spontaneously dirty themselves without ever brelse being called.
   We will ultimately want to put these in a separate list, but for
   now we search all of the lists for dirty buffers */

static int sync_buffers(dev_t dev, int wait)
{
	int i, retry, pass = 0, err = 0;
	int nlist, ncount;
	struct buffer_head * bh, *next;

	/* One pass for no-wait, three for wait:
	   0) write out all dirty, unlocked buffers;
	   1) write out all dirty buffers, waiting if locked;
	   2) wait for completion by waiting for all buffers to unlock. */
 repeat:
	retry = 0;
 repeat2:
	ncount = 0;
	/* We search all lists as a failsafe mechanism, not because we expect
	   there to be dirty buffers on any of the other lists. */
	for(nlist = 0; nlist < NR_LIST; nlist++)
	 {
	 repeat1:
		 bh = lru_list[nlist];
		 if(!bh) continue;
		 for (i = nr_buffers_type[nlist]*2 ; i-- > 0 ; bh = next) {
			 if(bh->b_list != nlist) goto repeat1;
			 next = bh->b_next_free;
			 if(!lru_list[nlist]) break;
			 if (dev && bh->b_dev != dev)
				  continue;
			 if (bh->b_lock)
			  {
				  /* Buffer is locked; skip it unless wait is
				     requested AND pass > 0. */
				  if (!wait || !pass) {
					  retry = 1;
					  continue;
				  }
				  wait_on_buffer (bh);
				  goto repeat2;
			  }
			 /* If an unlocked buffer is not uptodate, there has
			     been an IO error. Skip it. */
			 if (wait && bh->b_req && !bh->b_lock &&
			     !bh->b_dirt && !bh->b_uptodate) {
				  err = 1;
				  printk("Weird - unlocked, clean and not uptodate buffer on list %d %x %lu\n", nlist, bh->b_dev, bh->b_blocknr);
				  continue;
			  }
			 /* Don't write clean buffers.  Don't write ANY buffers
			    on the third pass. */
			 if (!bh->b_dirt || pass>=2)
				  continue;
			 /* don't bother about locked buffers */
			 if (bh->b_lock)
				 continue;
			 bh->b_count++;
			 bh->b_flushtime = 0;
			 ll_rw_block(WRITE, 1, &bh);

			 if(nlist != BUF_DIRTY) { 
				 printk("[%d %x %ld] ", nlist, bh->b_dev, bh->b_blocknr);
				 ncount++;
			 };
			 bh->b_count--;
			 retry = 1;
		 }
	 }
	if (ncount) printk("sys_sync: %d dirty buffers not on dirty list\n", ncount);
	
	/* If we are waiting for the sync to succeed, and if any dirty
	   blocks were written, then repeat; on the second pass, only
	   wait for buffers being written (do not pass to write any
	   more buffers on the second pass). */
	if (wait && retry && ++pass<=2)
		 goto repeat;
	return err;
}


int fsync_dev(dev_t dev)
{
	sync_buffers(dev, 0);
	sync_supers(dev);
	sync_inodes(dev);
	return sync_buffers(dev, 1);
}


/* ====================== bdflush support =================== */

/* This is a simple kernel daemon, whose job it is to provide a dynamically
 * response to dirty buffers.  Once this process is activated, we write back
 * a limited number of buffers to the disks and then go back to sleep again.
 * In effect this is a process which never leaves kernel mode, and does not have
 * any user memory associated with it except for the stack.  There is also
 * a kernel stack page, which obviously must be separate from the user stack.
 */
DECLARE_WAIT_QUEUE_HEAD(bdflush_wait);
DECLARE_WAIT_QUEUE_HEAD(bdflush_done);

static int bdflush_running = 0;

static void wakeup_bdflush(int wait)
{
	if(!bdflush_running){
		printk("Warning - bdflush not running\n");
		sync_buffers(0,0);
		return;
	};
	wake_up(&bdflush_wait);
	if(wait) sleep_on(&bdflush_done);
}




/* =========== Reduce the buffer memory ============= */

/*
 * try_to_free() checks if all the buffers on this particular page
 * are unused, and free's the page if so.
 */
static int try_to_free(struct buffer_head * bh, struct buffer_head ** bhp)
{
	unsigned long page;
	struct buffer_head * tmp, * p;
        int isize = BUFSIZE_INDEX(bh->b_size);

	*bhp = bh;
	page = (unsigned long) bh->b_data;
	page &= PAGE_MASK;
	tmp = bh;
	do {
		if (!tmp)
			return 0;
		if (tmp->b_count || tmp->b_dirt || tmp->b_lock || waitqueue_active(&tmp->b_wait))
			return 0;
		tmp = tmp->b_this_page;
	} while (tmp != bh);
	tmp = bh;
	do {
		p = tmp;
		tmp = tmp->b_this_page;
		nr_buffers--;
		nr_buffers_size[isize]--;
		if (p == *bhp)
		  {
		    *bhp = p->b_prev_free;
		    if (p == *bhp) /* Was this the last in the list? */
		      *bhp = NULL;
		  }
		remove_from_queues(p);
		put_unused_buffer_head(p);
	} while (tmp != bh);
	buffermem -= PAGE_SIZE;
	//buffer_pages[MAP_NR(page)] = NULL;
	dim_sum_pages_free((void *)page);
#ifdef xby_debug
	return !mem_map[MAP_NR(page)];
#else
	return 0;
#endif
}

static int shrink_specific_buffers(unsigned int priority, int size)
{
	struct buffer_head *bh;
	int nlist;
	int i, isize, isize1;

#ifdef DEBUG
	if(size) printk("Shrinking buffers of size %d\n", size);
#endif
	/* First try the free lists, and see if we can get a complete page
	   from here */
	isize1 = (size ? BUFSIZE_INDEX(size) : -1);

	for(isize = 0; isize<NR_SIZES; isize++){
		if(isize1 != -1 && isize1 != isize) continue;
		bh = free_list[isize];
		if(!bh) continue;
		for (i=0 ; !i || bh != free_list[isize]; bh = bh->b_next_free, i++) {
			if (bh->b_count || !bh->b_this_page)
				 continue;
			if (try_to_free(bh, &bh))
				 return 1;
			if(!bh) break; /* Some interrupt must have used it after we
					  freed the page.  No big deal - keep looking */
		}
	}
	
	/* Not enough in the free lists, now try the lru list */
	
	for(nlist = 0; nlist < NR_LIST; nlist++) {
	repeat1:
		if(priority > 3 && nlist == BUF_SHARED) continue;
		bh = lru_list[nlist];
		if(!bh) continue;
		i = 2*nr_buffers_type[nlist] >> priority;
		for ( ; i-- > 0 ; bh = bh->b_next_free) {
			/* We may have stalled while waiting for I/O to complete. */
			if(bh->b_list != nlist) goto repeat1;
			if (bh->b_count || !bh->b_this_page)
				 continue;
			if(size && bh->b_size != size) continue;
			if (bh->b_lock)
			{
				if (priority)
				{
					continue;
				}
				else
				{
					wait_on_buffer(bh);
				}
			}
			if (bh->b_dirt) {
				bh->b_count++;
				bh->b_flushtime = 0;
				ll_rw_block(WRITEA, 1, &bh);
				bh->b_count--;
				continue;
			}
			if (try_to_free(bh, &bh))
				 return 1;
			if(!bh) break;
		}
	}
	return 0;
}

/*
 * Consult the load average for buffers and decide whether or not
 * we should shrink the buffers of one size or not.  If we decide yes,
 * do it and return 1.  Else return 0.  Do not attempt to shrink size
 * that is specified.
 *
 * I would prefer not to use a load average, but the way things are now it
 * seems unavoidable.  The way to get rid of it would be to force clustering
 * universally, so that when we reclaim buffers we always reclaim an entire
 * page.  Doing this would mean that we all need to move towards QMAGIC.
 */

static int maybe_shrink_lav_buffers(int size)
{	   
	int nlist;
	int isize;
	int total_lav, total_n_buffers, n_sizes;
	
	/* Do not consider the shared buffers since they would not tend
	   to have getblk called very often, and this would throw off
	   the lav.  They are not easily reclaimable anyway (let the swapper
	   make the first move). */
  
	total_lav = total_n_buffers = n_sizes = 0;
	for(nlist = 0; nlist < NR_SIZES; nlist++)
	 {
		 total_lav += buffers_lav[nlist];
		 if(nr_buffers_size[nlist]) n_sizes++;
		 total_n_buffers += nr_buffers_size[nlist];
		 total_n_buffers -= nr_buffers_st[nlist][BUF_SHARED]; 
	 }
	
	/* See if we have an excessive number of buffers of a particular
	   size - if so, victimize that bunch. */
  
	isize = (size ? BUFSIZE_INDEX(size) : -1);
	
	if (n_sizes > 1)
		 for(nlist = 0; nlist < NR_SIZES; nlist++)
		  {
			  if(nlist == isize) continue;
			  if(nr_buffers_size[nlist] &&
			     bdf_prm.b_un.lav_const * buffers_lav[nlist]*total_n_buffers < 
			     total_lav * (nr_buffers_size[nlist] - nr_buffers_st[nlist][BUF_SHARED]))
				   if(shrink_specific_buffers(6, bufferindex_size[nlist])) 
					    return 1;
		  }
	return 0;
}

/*
 * Try to increase the number of buffers available: the size argument
 * is used to determine what kind of buffers we want.
 */
static int grow_buffers(int pri, int size)
{
	unsigned long page;
	struct buffer_head *bh, *tmp;
	struct buffer_head * insert_point;
	int isize;

	if ((size & 511) || (size > PAGE_SIZE)) {
		printk("VFS: grow_buffers: size = %d\n",size);
		return 0;
	}

	isize = BUFSIZE_INDEX(size);

	if (!(page = (unsigned long)get_free_buffer_page(pri)))
		return 0;
	bh = create_buffers(page, size);
	if (!bh) {
		free_buffer_page(page);
		return 0;
	}

	insert_point = free_list[isize];

	tmp = bh;
	while (1) {
	    nr_free[isize]++;
		if (insert_point) {
			tmp->b_next_free = insert_point->b_next_free;
			tmp->b_prev_free = insert_point;
			insert_point->b_next_free->b_prev_free = tmp;
			insert_point->b_next_free = tmp;
		} else {
			tmp->b_prev_free = tmp;
			tmp->b_next_free = tmp;
		}
		insert_point = tmp;
		++nr_buffers;
		if (tmp->b_this_page)
			tmp = tmp->b_this_page;
		else
			break;
	}
	free_list[isize] = bh;
	//buffer_pages[MAP_NR(page)] = bh;
	tmp->b_this_page = bh;
	wake_up(&buffer_wait);
	buffermem += PAGE_SIZE;
	return 1;
}



/* ===================== Init ======================= */

/*
 * This initializes the initial buffer free list.  nr_buffers_type is set
 * to one less the actual number of buffers, as a sop to backwards
 * compatibility --- the old code did this (I think unintentionally,
 * but I'm not sure), and programs in the ps package expect it.
 * 					- TYT 8/30/92
 */
void buffer_init_early(void)
{
	int i;
	

#ifdef xby_debug
	if (high_memory >= 4*1024*1024) {
		if(high_memory >= 16*1024*1024)
			 nr_hash = 16381;
		else
			 nr_hash = 4093;
	} else {
		nr_hash = 997;
	};
#else
	nr_hash = 16381;
#endif

#ifdef xby_debug
	hash_table = (struct buffer_head **) vmalloc(nr_hash * 
						     sizeof(struct buffer_head *));


	buffer_pages = (struct buffer_head **) vmalloc(MAP_NR(high_memory) * 
						     sizeof(struct buffer_head *));


	for (i = 0 ; i < MAP_NR(high_memory) ; i++)
		buffer_pages[i] = NULL;
#else
	hash_table = (struct buffer_head **) AllocPermanentBootMemory(BOOT_MEMORY_MODULE_IO, nr_hash * 
						     sizeof(struct buffer_head *), 0);
#endif

	for (i = 0 ; i < nr_hash ; i++)
		hash_table[i] = NULL;
	lru_list[BUF_CLEAN] = 0;

	return;
}

void buffer_init_tail(void)
{
	int isize = BUFSIZE_INDEX(BLOCK_SIZE);

	init_buffer_page();
	grow_buffers(MEM_NORMAL, BLOCK_SIZE);
	if (!free_list[isize])
		panic("VFS: Unable to initialize buffer free list!");
	return;
}

int file_fsync (struct inode *inode, struct file *filp)
{
	return fsync_dev(inode->i_dev);
}


void invalidate_buffers(dev_t dev)
{
	int i;
	int nlist;
	struct buffer_head * bh;

	for(nlist = 0; nlist < NR_LIST; nlist++) {
		bh = lru_list[nlist];
		for (i = nr_buffers_type[nlist]*2 ; --i > 0 ; bh = bh->b_next_free) {
			if (bh->b_dev != dev)
				continue;
			wait_on_buffer(bh);
			if (bh->b_dev != dev)
				continue;
			if (bh->b_count)
				continue;
			bh->b_flushtime = bh->b_uptodate = 
				bh->b_dirt = bh->b_req = 0;
		}
	}
}


void set_blocksize(dev_t dev, int size)
{
	int i, nlist;
	struct buffer_head * bh, *bhnext;

	if (!blksize_size[MAJOR(dev)])
		return;

	switch(size) {
		default: panic("Invalid blocksize passed to set_blocksize");
		case 512: case 1024: case 2048: case 4096:;
	}

	if (blksize_size[MAJOR(dev)][MINOR(dev)] == 0 && size == BLOCK_SIZE) {
		blksize_size[MAJOR(dev)][MINOR(dev)] = size;
		return;
	}
	if (blksize_size[MAJOR(dev)][MINOR(dev)] == size)
		return;
	sync_buffers(dev, 2);
	blksize_size[MAJOR(dev)][MINOR(dev)] = size;

  /* We need to be quite careful how we do this - we are moving entries
     around on the free list, and we can get in a loop if we are not careful.*/

	for(nlist = 0; nlist < NR_LIST; nlist++) {
		bh = lru_list[nlist];
		for (i = nr_buffers_type[nlist]*2 ; --i > 0 ; bh = bhnext) {
			if(!bh) break;
			bhnext = bh->b_next_free; 
			if (bh->b_dev != dev)
				 continue;
			if (bh->b_size == size)
				 continue;
			
			wait_on_buffer(bh);
			if (bh->b_dev == dev && bh->b_size != size) {
				bh->b_uptodate = bh->b_dirt = bh->b_req =
					 bh->b_flushtime = 0;
			};
			remove_from_hash_queue(bh);
		}
	}
}

asmlinkage int sys_fsync(unsigned int fd)
{
	struct file * file;
	struct inode * inode;

	if (fd>=NR_OPEN || !(file=current->files->fd_array[fd]) || !(inode=file->f_inode))
		return -EBADF;
	if (!file->f_op || !file->f_op->fsync)
		return -EINVAL;
	if (file->f_op->fsync(inode,file))
		return -EIO;
	return 0;
}


asmlinkage int sys_sync(void)
{
	sync_dev(0);
	return 0;
}


void sync_dev(dev_t dev)
{
	sync_buffers(dev, 0);
	sync_supers(dev);
	sync_inodes(dev);
	sync_buffers(dev, 0);
}

