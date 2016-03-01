#ifndef _BLK_H
#define _BLK_H
#include <fs/blk_dev.h>


/*
 * NR_REQUEST is the number of entries in the request-queue.
 * NOTE that writes may use only the low 2/3 of these: reads
 * take precedence.
 */
#define NR_REQUEST	64

#ifdef MAJOR_NR
#if (MAJOR_NR == MEM_MAJOR)
/* ram disk */
#define DEVICE_NAME "ramdisk"
#define DEVICE_REQUEST do_rd_request
#define DEVICE_NR(device) ((device) & 7)
#define DEVICE_ON(device) 
#define DEVICE_OFF(device)

#elif (MAJOR_NR == SD_MAJOR)
/* sd disk */
#define DEVICE_NAME "sd-disk"
#define DEVICE_REQUEST do_sd_request
#define DEVICE_NR(device) ((device) & 7)
#define DEVICE_ON(device) 
#define DEVICE_OFF(device)

#endif
#endif

#define DK_NDRIVE 4

struct kernel_stat {
	unsigned int cpu_user, cpu_nice, cpu_system;
	unsigned int dk_drive[DK_NDRIVE];
	unsigned int pgpgin, pgpgout;
	unsigned int pswpin, pswpout;
	unsigned int interrupts[16];
	unsigned int ipackets, opackets;
	unsigned int ierrors, oerrors;
	unsigned int collisions;
	unsigned int context_swtch;
};

extern struct kernel_stat kstat;

/*
 * This is used in the elevator algorithm: Note that
 * reads always go before writes. This is natural: reads
 * are much more time-critical than writes.
 */
#define IN_ORDER(s1,s2) \
((s1)->cmd < (s2)->cmd || ((s1)->cmd == (s2)->cmd && \
((s1)->dev < (s2)->dev || (((s1)->dev == (s2)->dev && \
(s1)->sector < (s2)->sector)))))
#if defined(MAJOR_NR) || defined(IDE_DRIVER)

#ifdef IDE_DRIVER
#define SECTOR_MASK ((BLOCK_SIZE >> 9) - 1)
#else
#define SECTOR_MASK (blksize_size[MAJOR_NR] &&     \
	blksize_size[MAJOR_NR][MINOR(CURRENT->dev)] ? \
	((blksize_size[MAJOR_NR][MINOR(CURRENT->dev)] >> 9) - 1) :  \
	((BLOCK_SIZE >> 9)  -  1))
#endif /* IDE_DRIVER */

#define SUBSECTOR(block) (CURRENT->current_nr_sectors > 0)


#if (MAJOR_NR != SCSI_TAPE_MAJOR) && !defined(IDE_DRIVER)

#ifndef CURRENT
#define CURRENT (blk_dev[MAJOR_NR].current_request)
#endif

#define CURRENT_DEV DEVICE_NR(CURRENT->dev)

#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;
#endif
#ifdef DEVICE_TIMEOUT

#define SET_TIMER \
((timer_table[DEVICE_TIMEOUT].expires = jiffies + TIMEOUT_VALUE), \
(timer_active |= 1<<DEVICE_TIMEOUT))

#define CLEAR_TIMER \
timer_active &= ~(1<<DEVICE_TIMEOUT)

#define SET_INTR(x) \
if ((DEVICE_INTR = (x)) != NULL) \
	SET_TIMER; \
else \
	CLEAR_TIMER;

#else

#define SET_INTR(x) (DEVICE_INTR = (x))

#endif /* DEVICE_TIMEOUT */

static void (DEVICE_REQUEST)(void);

#ifdef DEVICE_INTR
#define CLEAR_INTR SET_INTR(NULL)
#else
#define CLEAR_INTR
#endif

#define INIT_REQUEST \
	if (!CURRENT) {\
		CLEAR_INTR; \
		return; \
	} \
	if (MAJOR(CURRENT->dev) != MAJOR_NR) \
		panic(DEVICE_NAME ": request list destroyed"); \
	if (CURRENT->bh) { \
		if (!CURRENT->bh->b_lock) \
			panic(DEVICE_NAME ": block not locked"); \
	}

#endif /* (MAJOR_NR != SCSI_TAPE_MAJOR) && !defined(IDE_DRIVER) */

/* end_request() - SCSI devices have their own version */

#if ! SCSI_MAJOR(MAJOR_NR)

#ifdef IDE_DRIVER
static void end_request(byte uptodate, byte hwif) {
	struct request *req = ide_cur_rq[HWIF];
#else
static void end_request(int uptodate) {
	struct request *req = CURRENT;
#endif /* IDE_DRIVER */
	struct buffer_head * bh;

	req->errors = 0;
	if (!uptodate) {
		printk("end_request: I/O error, dev %04lX, sector %lu\n",
		       (unsigned long)req->dev, req->sector);
		req->nr_sectors--;
		req->nr_sectors &= ~SECTOR_MASK;
		req->sector += (BLOCK_SIZE / 512);
		req->sector &= ~SECTOR_MASK;		
	}

	if ((bh = req->bh) != NULL) {
		req->bh = bh->b_reqnext;
		bh->b_reqnext = NULL;
		bh->b_uptodate = uptodate;		
		if (!uptodate) bh->b_req = 0; /* So no "Weird" errors */
		unlock_buffer(bh);
		if ((bh = req->bh) != NULL) {
			req->current_nr_sectors = bh->b_size >> 9;
			if (req->nr_sectors < req->current_nr_sectors) {
				req->nr_sectors = req->current_nr_sectors;
				printk("end_request: buffer-list destroyed\n");
			}
			req->buffer = bh->b_data;
			return;
		}
	}
#ifdef IDE_DRIVER
	ide_cur_rq[HWIF] = NULL;
#else
	DEVICE_OFF(req->dev);
	CURRENT = req->next;
#endif /* IDE_DRIVER */
#if 0
	if (req->sem != NULL)
		up(req->sem);
#endif
	req->dev = -1;
	wake_up(&wait_for_request);
}
#endif /* ! SCSI_MAJOR(MAJOR_NR) */

#endif /* defined(MAJOR_NR) || defined(IDE_DRIVER) */


#endif
