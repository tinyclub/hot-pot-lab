#include <base/bitops.h>
#include <base/string.h>
#include <dim-sum/boot_allotter.h>
#include <fs/fs.h>
#include <linux/types.h>
//#include <linux/stat.h>
#include <linux/time.h>
#include <linux/math64.h>

//根文件系统大小，暂定为4M
int root_ramdisk_size = 4 * 1024 * 1024;
//根文件系统起始位置
char *root_ramdisk_addr;

#ifndef malloc
#define malloc(s)	AllocPermanentBootMemory(BOOT_MEMORY_MODULE_UNKNOWN, s, 0)
#endif
	
#ifndef free
#define free(p)		FreeBootMemory(p, 0)
#endif

#define TRUE 1			/* Boolean constants */
#define FALSE 0

#define TEST_BUFFER_BLOCKS       16
#define HARD_SECTOR_SIZE         512
#define SECTORS_PER_BLOCK        ( BLOCK_SIZE >> 9 )

#define NO_NAME "NO NAME    "

#define BOOTCODE_SIZE		          448
#define BOOTCODE_FAT32_SIZE	      420

/* MS-DOS filesystem structures -- I included them here instead of
   including linux/msdos_fs.h since that doesn't include some fields we
   need */

#define ATTR_RO      1		/* read-only */
#define ATTR_HIDDEN  2		/* hidden */
#define ATTR_SYS     4		/* system */
#define ATTR_VOLUME  8		/* volume label */
#define ATTR_DIR     16		/* directory */
#define ATTR_ARCH    32		/* archived */

#define ATTR_NONE    0		/* no attribute bits */
#define ATTR_UNUSED  (ATTR_VOLUME | ATTR_ARCH | ATTR_SYS | ATTR_HIDDEN)
/* attribute bits that are copied "as is" */
/* FAT values */
#define FAT_EOF      0x0ffffff8
#define FAT_BAD      0x0ffffff7
#define BOOT_SIGN                 0xAA55	    /* Boot sector magic number */
#define MSDOS_FAT12_SIGN          "FAT12   "	/* FAT12 filesystem signature */
#define MSDOS_FAT16_SIGN          "FAT16   "	/* FAT16 filesystem signature */
#define MSDOS_FAT32_SIGN          "FAT32   "	/* FAT32 filesystem signature */
#define MSDOS_EXT_SIGN            0x29	      /* extended boot sector signature */
#define FAT12_THRESHOLD	          4085
#define MAX_CLUST_12	            ((1 << 12) - 16)
#define MAX_CLUST_16	            ((1 << 16) - 16)
#define MIN_CLUST_32              65529
/* M$ says the high 4 bits of a FAT32 FAT entry are reserved and don't belong
 * to the cluster number. So the max. cluster# is based on 2^28 */
#define MAX_CLUST_32	            ((1 << 28) - 16)


/*
 * Similar to the struct tm in userspace <time.h>, but it needs to be here so
 * that the kernel source is self contained.
 */
struct tm {
	/*
	 * the number of seconds after the minute, normally in the range
	 * 0 to 59, but can be up to 60 to allow for leap seconds
	 */
	int tm_sec;
	/* the number of minutes after the hour, in the range 0 to 59*/
	int tm_min;
	/* the number of hours past midnight, in the range 0 to 23 */
	int tm_hour;
	/* the day of the month, in the range 1 to 31 */
	int tm_mday;
	/* the number of months since January, in the range 0 to 11 */
	int tm_mon;
	/* the number of years since 1900 */
	long tm_year;
	/* the number of days since Sunday, in the range 0 to 6 */
	int tm_wday;
	/* the number of days since January 1, in the range 0 to 365 */
	int tm_yday;
};

/* __attribute__ ((packed)) is used on all structures to make gcc ignore any
 * alignments */
struct msdos_volume_info {
    unsigned char drive_number;		/* BIOS drive number */
    unsigned char RESERVED;		/* Unused */
    unsigned char ext_boot_sign;		/* 0x29 if fields below exist (DOS 3.3+) */
    unsigned char volume_id[4];		/* Volume ID number */
    unsigned char volume_label[11];	/* Volume label */
    unsigned char fs_type[8];		/* Typically FAT12 or FAT16 */
} __attribute__ ((packed));

struct msdos_boot_sector {
    unsigned char boot_jump[3];		/* Boot strap short or near jump */
    unsigned char system_id[8];		/* Name - can be used to special case partition manager volumes */
    unsigned char sector_size[2];	/* bytes per logical sector */
    unsigned char cluster_size;		/* sectors/cluster */
    unsigned short reserved;		/* reserved sectors */
    unsigned char fats;			/* number of FATs */
    unsigned char dir_entries[2];	/* root directory entries */
    unsigned char sectors[2];		/* number of sectors */
    unsigned char media;			/* media code (unused) */
    unsigned short fat_length;		/* sectors/FAT */
    unsigned short secs_track;		/* sectors per track */
    unsigned short heads;		/* number of heads */
    unsigned short hidden;		/* hidden sectors (unused) */
    unsigned int total_sect;		/* number of sectors (if sectors == 0) */
    union {
	struct {
	    struct msdos_volume_info vi;
	    unsigned char boot_code[BOOTCODE_SIZE];
	} __attribute__ ((packed)) _oldfat;
	struct {
	    unsigned int fat32_length;	/* sectors/FAT */
	    unsigned short flags;	/* bit 8: fat mirroring, low 4: active fat */
	    unsigned char version[2];	/* major, minor filesystem version */
	    unsigned int root_cluster;	/* first cluster in root directory */
	    unsigned short info_sector;	/* filesystem info sector */
	    unsigned short backup_boot;	/* backup boot sector */
	    unsigned short reserved2[6];	/* Unused */
	    struct msdos_volume_info vi;
	    unsigned char boot_code[BOOTCODE_FAT32_SIZE];
	} __attribute__ ((packed)) _fat32;
    } __attribute__ ((packed)) fstype;
    unsigned short boot_sign;
} __attribute__ ((packed));
#define fat32	fstype._fat32
#define oldfat	fstype._oldfat

struct fat32_fsinfo {
    unsigned int reserved1;		/* Nothing as far as I can tell */
    unsigned int signature;		/* 0x61417272L */
    unsigned int free_clusters;	/* Free cluster count.  -1 if unknown */
    unsigned int next_cluster;		/* Most recently allocated cluster.
				 * Unused under Linux. */
    unsigned int reserved2[4];
};

struct msdos_dir_entry {
    char name[8], ext[3];	/* name and extension */
    unsigned char attr;			/* attribute bits */
    unsigned char lcase;			/* Case for base and extension */
    unsigned char ctime_ms;		/* Creation time, milliseconds */
    unsigned short ctime;		/* Creation time */
    unsigned short cdate;		/* Creation date */
    unsigned short adate;		/* Last access date */
    unsigned short starthi;		/* high 16 bits of first cl. (FAT32) */
    unsigned short time, date, start;	/* time, date and first cluster */
    unsigned int size;			/* file size (in bytes) */
} __attribute__ ((packed));
static char *device_name = NULL;	/* Name of the device on which to create the filesystem */
static long volume_id;		            /* Volume ID number */
static char *volume_name;	/* Volume name */
static struct msdos_boot_sector bs;   /* Boot sector data */
static int blocks;               /* Number of blocks in filesystem */
static int start_data_sector;	/* Sector number for the start of the data area */
static int start_data_block;	/* Block number for the start of the data area */
static int nr_fats = 2;		            /* Default number of FATs to produce */
static int size_fat = 0;	            /* Size in bits of FAT entries */
static int size_fat_by_user = 0;	/* 1 if FAT size user selected */
static int sector_size = 512;	        /* Size of a logical sector */
static int backup_boot = 0;	          /* Sector# of backup boot sector */
static int reserved_sectors = 0;	    /* Number of reserved sectors */
static int hidden_sectors = 0;	          /* Number of hidden sectors */
static int orphaned_sectors = 0;	        /* Sectors that exist in the last block of filesystem */
static int align_structures = TRUE;	      /* Whether to enforce alignment */
static int malloc_entire_fat = FALSE;	    /* Whether we should malloc() the entire FAT or not */
static unsigned char *fat;	              /* File allocation table */
static unsigned alloced_fat_length;	      /* # of FAT sectors we can keep in memory */
static unsigned char *info_sector;	      /* FAT32 info sector */
static struct msdos_dir_entry *root_dir;	/* Root directory */
static int size_root_dir;	                /* Size of the root directory in bytes */
static int sectors_per_cluster = 0;	      /* Number of sectors per disk cluster */
static int root_dir_entries = 0;	        /* Number of root directory entries */
static char *blank_sector;	              /* Blank sector - all zeros */



static int verbose = 0;		                /* Default to verbose mode off */
char dummy_boot_jump[3] = { 0xeb, 0x3c, 0x90 };

#define MSG_OFFSET_OFFSET 3
char dummy_boot_code[BOOTCODE_SIZE] = "\x0e"	/* push cs */
    "\x1f"			/* pop ds */
    "\xbe\x5b\x7c"		/* mov si, offset message_txt */
    /* write_msg: */
    "\xac"			/* lodsb */
    "\x22\xc0"			/* and al, al */
    "\x74\x0b"			/* jz key_press */
    "\x56"			/* push si */
    "\xb4\x0e"			/* mov ah, 0eh */
    "\xbb\x07\x00"		/* mov bx, 0007h */
    "\xcd\x10"			/* int 10h */
    "\x5e"			/* pop si */
    "\xeb\xf0"			/* jmp write_msg */
    /* key_press: */
    "\x32\xe4"			/* xor ah, ah */
    "\xcd\x16"			/* int 16h */
    "\xcd\x19"			/* int 19h */
    "\xeb\xfe"			/* foo: jmp foo */
    /* message_txt: */
    "This is not a bootable disk.  Please insert a bootable floppy and\r\n"
    "press any key to try again ... \r\n";

#define MESSAGE_OFFSET 29	/* Offset of message in above code */

/* Compute ceil(a/b) */
static inline int cdiv(int a, int b)
{
    return (a + b - 1) / b;
}


/* Mark the specified cluster as having a particular value */

static void mark_FAT_cluster(int cluster, unsigned int value)
{
  switch (size_fat) {
  case 12:
		value &= 0x0fff;
		if (((cluster * 3) & 0x1) == 0) {
	    fat[3 * cluster / 2] = (unsigned char)(value & 0x00ff);
	    fat[(3 * cluster / 2) + 1] =
		(unsigned char)((fat[(3 * cluster / 2) + 1] & 0x00f0)
				| ((value & 0x0f00) >> 8));
		} else {
	    fat[3 * cluster / 2] =
		(unsigned char)((fat[3 * cluster / 2] & 0x000f) |
				((value & 0x000f) << 4));
	    fat[(3 * cluster / 2) + 1] = (unsigned char)((value & 0x0ff0) >> 4);
		}
		break;

  case 16:
		value &= 0xffff;
		fat[2 * cluster] = (unsigned char)(value & 0x00ff);
		fat[(2 * cluster) + 1] = (unsigned char)(value >> 8);
		break;

  case 32:
		value &= 0xfffffff;
		fat[4 * cluster] = (unsigned char)(value & 0x000000ff);
		fat[(4 * cluster) + 1] = (unsigned char)((value & 0x0000ff00) >> 8);
		fat[(4 * cluster) + 2] = (unsigned char)((value & 0x00ff0000) >> 16);
		fat[(4 * cluster) + 3] = (unsigned char)((value & 0xff000000) >> 24);
		break;

  default:
		BUG();
  }
}


/* Establish the geometry and media parameters for the device */
static void establish_params(void)
{
	int def_root_dir_entries = 512;

	memset(&bs, 0, sizeof(bs));
	/* Must be a hard disk then! */
	/* If we can't get the drive geometry. using default 255/6 */
	bs.secs_track = cpu_to_le16(63);
	bs.heads = cpu_to_le16(255);
	bs.media = (char)0xf8;	/* Set up the media descriptor for a hard drive */
    
	if (!size_fat && blocks * SECTORS_PER_BLOCK > 1064960) {
		if (verbose)
			printk("Auto-selecting FAT32 for large filesystem\n");
		size_fat = 32;
	}
    if (size_fat == 32) {
        /* For FAT32, try to do the same as M$'s format command
         * (http://technet.microsoft.com/en-us/library/cc938438.aspx):
         * fs size <   8G:  4k clusters
         * fs size <  16G:  8k clusters
         * fs size <  32G: 16k clusters
         * fs size >= 32G: 32k clusters
         */ 
         uint32_t sz_mb = (blocks + (1 << (20 - BLOCK_SIZE_BITS)) - 1) >> (20 -
								          BLOCK_SIZE_BITS);
         bs.cluster_size = sz_mb >= 32 * 1024 ? 64 : sz_mb >= 16 * 1024 ? 32 : sz_mb >=
                           8 * 1024 ? 16 : sz_mb > 260 ? 8 : 1;
    } else {
         /* FAT12 and FAT16: start at 4 sectors per cluster */
         bs.cluster_size = (char)4;
    }

    if (!root_dir_entries)
    		root_dir_entries = def_root_dir_entries;
}

/*
 * If alignment is enabled, round the first argument up to the second; the
 * latter must be a power of two.
 */
static unsigned int align_object(unsigned int sectors, unsigned int clustsize)
{
	if (align_structures)
		return (sectors + clustsize - 1) & ~(clustsize - 1);
	else
		return sectors;
}

static struct tm ctime;
/* Create the filesystem data tables */
static void setup_tables(void)
{
	unsigned num_sectors;
	unsigned cluster_count = 0, fat_length;

	struct msdos_volume_info *vi = (size_fat == 32 ? &bs.fat32.vi : &bs.oldfat.vi);
	unsigned fatdata1216;	/* Sectors for FATs + data area (FAT12/16) */
	unsigned fatdata32;	/* Sectors for FATs + data area (FAT32) */
	unsigned fatlength12, fatlength16, fatlength32;
	unsigned maxclust12, maxclust16, maxclust32;
	unsigned clust12, clust16, clust32;
	int maxclustsize;
	unsigned root_dir_sectors;

	memcpy((char *)bs.system_id, "mkfs.fat", strlen("mkfs.fat"));
	if (sectors_per_cluster)
		bs.cluster_size = (char)sectors_per_cluster;

	if (bs.media == 0xf8)
		vi->drive_number=0x80;
	else
		vi->drive_number=0x00;

	if (size_fat == 32) {
		/* Under FAT32, the root dir is in a cluster chain, and this is signalled by bs.dir_entries being 0. */
		root_dir_entries = 0;
	}
	vi->volume_id[0] = (unsigned char)(volume_id & 0x000000ff);
	vi->volume_id[1] = (unsigned char)((volume_id & 0x0000ff00) >> 8);
	vi->volume_id[2] = (unsigned char)((volume_id & 0x00ff0000) >> 16);
	vi->volume_id[3] = (unsigned char)(volume_id >> 24);

	memcpy(vi->volume_label, volume_name, 11);
	memcpy(bs.boot_jump, dummy_boot_jump, 3);
	/* Patch in the correct offset to the boot code */
	bs.boot_jump[1] = ((size_fat == 32 ? (char *)&bs.fat32.boot_code : (char *)&bs.oldfat.boot_code) - (char *)&bs) - 2;

	if (size_fat == 32) {
		int offset = (char *)&bs.fat32.boot_code - (char *)&bs + MESSAGE_OFFSET + 0x7c00;
		if (dummy_boot_code[BOOTCODE_FAT32_SIZE - 1])
			printk("Warning: message too long; truncated\n");
		dummy_boot_code[BOOTCODE_FAT32_SIZE - 1] = 0;
		memcpy(bs.fat32.boot_code, dummy_boot_code, BOOTCODE_FAT32_SIZE);	  
		bs.fat32.boot_code[MSG_OFFSET_OFFSET] = offset & 0xff;
		bs.fat32.boot_code[MSG_OFFSET_OFFSET + 1] = offset >> 8;
	} else {
		memcpy(bs.oldfat.boot_code, dummy_boot_code, BOOTCODE_SIZE);
	}
	bs.boot_sign = cpu_to_le16(BOOT_SIGN);
	if (verbose)
		printk("Bs.boot_sign %x\n",(char *)&bs.fat32.boot_code - (char *)&bs);
 
	if (verbose)
		printk("Boot jump code is %02x %02x\n",bs.boot_jump[0], bs.boot_jump[1]);

	if (!reserved_sectors)
		reserved_sectors = (size_fat == 32) ? 32 : 1;
	else {
		if (size_fat == 32 && reserved_sectors < 2)
		BUG();
	}
	bs.reserved = cpu_to_le16(reserved_sectors);    
	if (verbose)
		printk("Using %d reserved sectors\n", reserved_sectors);
    
	bs.fats = (char)nr_fats;
    
	if (size_fat == 32)
		bs.hidden = cpu_to_le32(hidden_sectors);
	else {
		unsigned short hidden = cpu_to_le16(hidden_sectors);
		if (hidden_sectors & ~0xffff)
			BUG();
		memcpy(&bs.hidden, &hidden, 2);
	}
	num_sectors = div_u64(blocks * BLOCK_SIZE , sector_size) + orphaned_sectors;
	//num_sectors = (long long)(blocks * BLOCK_SIZE / sector_size) + orphaned_sectors;
	root_dir_sectors = cdiv(root_dir_entries * 32, sector_size);

	/*
	 * If the filesystem is 8192 sectors or less (4 MB with 512-byte
	 * sectors, i.e. floppy size), don't align the data structures.
	 */
	if (num_sectors <= 8192) {
		if (align_structures && verbose)
			printk("Disabling alignment due to tiny filesystem\n");
		align_structures = FALSE;
	}

	if (sectors_per_cluster)
		bs.cluster_size = maxclustsize = sectors_per_cluster;
	else
		/* An initial guess for bs.cluster_size should already be set */
		maxclustsize = 128;

	do {
		fatdata32 = num_sectors - reserved_sectors;
		fatdata1216 = fatdata32 - align_object(root_dir_sectors, bs.cluster_size);

		if (verbose)
			printk("Trying with %d sectors/cluster:\n", bs.cluster_size);

		/* The factor 2 below avoids cut-off errors for nr_fats == 1.
		 * The "nr_fats*3" is for the reserved first two FAT entries */
		//clust12 = 2 * ((long long)fatdata1216 * sector_size + nr_fats * 3) /
		//						(2 * (int)bs.cluster_size * sector_size + nr_fats * 3);
		clust12 = div_u64(2 * ((long long)fatdata1216 * sector_size + nr_fats * 3),
						(2 * (int)bs.cluster_size * sector_size + nr_fats * 3));
		fatlength12 = cdiv(((clust12 + 2) * 3 + 1) >> 1, sector_size);
		fatlength12 = align_object(fatlength12, bs.cluster_size);
		/* Need to recalculate number of clusters, since the unused parts of the
		 * FATS and data area together could make up space for an additional,
		 * not really present cluster. */
		clust12 = (fatdata1216 - nr_fats * fatlength12) / bs.cluster_size;

		maxclust12 = (fatlength12 * 2 * sector_size) / 3;
		if (maxclust12 > MAX_CLUST_12)
			maxclust12 = MAX_CLUST_12;
		if (verbose)
			printk("FAT12: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n",clust12, fatlength12, maxclust12, MAX_CLUST_12);
		if (clust12 > maxclust12 - 2) {
			clust12 = 0;
			if (verbose)
			printk("FAT12: too much clusters\n");
	    }

	    //clust16 = ((long long)fatdata1216 * sector_size + nr_fats * 4) /
		//						((int)bs.cluster_size * sector_size + nr_fats * 2);
		clust16 = div_u64(((long long)fatdata1216 * sector_size + nr_fats * 4),
								((int)bs.cluster_size * sector_size + nr_fats * 2));
	    fatlength16 = cdiv((clust16 + 2) * 2, sector_size);
	    fatlength16 = align_object(fatlength16, bs.cluster_size);
	    /* Need to recalculate number of clusters, since the unused parts of the
	     * FATS and data area together could make up space for an additional,
	     * not really present cluster. */
	    clust16 = (fatdata1216 - nr_fats * fatlength16) / bs.cluster_size;
	    maxclust16 = (fatlength16 * sector_size) / 2;
	    if (maxclust16 > MAX_CLUST_16)
			maxclust16 = MAX_CLUST_16;
	    if (verbose)
			printk("FAT16: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n",clust16, fatlength16, maxclust16, MAX_CLUST_16);
	    if (clust16 > maxclust16 - 2) {
			if (verbose)
				printk("FAT16: too much clusters\n");
			clust16 = 0;
		}
	    /* The < 4078 avoids that the filesystem will be misdetected as having a
	     * 12 bit FAT. */
	    if (clust16 < FAT12_THRESHOLD && !(size_fat_by_user && size_fat == 16)) {
			if (verbose)
				printk(clust16 < FAT12_THRESHOLD ? "FAT16: would be misdetected as FAT12\n" : "FAT16: too much clusters\n");
			clust16 = 0;
		}

		//clust32 = ((long long)fatdata32 * sector_size + nr_fats * 8) / ((int)bs.cluster_size * sector_size + nr_fats * 4);
		clust32 = div_u64(((long long)fatdata32 * sector_size + nr_fats * 8),
	    			((int)bs.cluster_size * sector_size + nr_fats * 4));
		fatlength32 = cdiv((clust32 + 2) * 4, sector_size);
		/* Need to recalculate number of clusters, since the unused parts of the
	     * FATS and data area together could make up space for an additional,
	     * not really present cluster. */
		clust32 = (fatdata32 - nr_fats * fatlength32) / bs.cluster_size;
		maxclust32 = (fatlength32 * sector_size) / 4;
		if (maxclust32 > MAX_CLUST_32)
			maxclust32 = MAX_CLUST_32;
		if (clust32 && clust32 < MIN_CLUST_32 && !(size_fat_by_user && size_fat == 32)) {
			clust32 = 0;
			if (verbose)
				printk("FAT32: not enough clusters (%d)\n", MIN_CLUST_32);
	    }
		if (verbose)
			printk("FAT32: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n", clust32, fatlength32, maxclust32, MAX_CLUST_32);
		if (clust32 > maxclust32) {
			clust32 = 0;
			if (verbose)
				printk("FAT32: too much clusters\n");
		}

		if ((clust12 && (size_fat == 0 || size_fat == 12)) ||
					(clust16 && (size_fat == 0 || size_fat == 16)) ||
					(clust32 && size_fat == 32))
			break;

		bs.cluster_size <<= 1;
		printk("bs.cluster_size %d, maxclustsize %d\n", bs.cluster_size, maxclustsize);
	} while (bs.cluster_size && bs.cluster_size <= maxclustsize);

	/* Use the optimal FAT size if not specified;
	 * FAT32 is (not yet) choosen automatically */
	if (!size_fat) {
		size_fat = (clust16 > clust12) ? 16 : 12;
		if (verbose)
			printk("Choosing %d bits for FAT\n", size_fat);
	}
	switch (size_fat) {
	case 12:
		cluster_count = clust12;
		fat_length = fatlength12;
		bs.fat_length = cpu_to_le16(fatlength12);
		memcpy(vi->fs_type, MSDOS_FAT12_SIGN, 8);
		break;

	case 16:
		if (clust16 < FAT12_THRESHOLD) {
			BUG();
		}
		cluster_count = clust16;
		fat_length = fatlength16;
		bs.fat_length = cpu_to_le16(fatlength16);
		memcpy(vi->fs_type, MSDOS_FAT16_SIGN, 8);
		break;

	case 32:
		if (clust32 < MIN_CLUST_32)
		{
			BUG();
		}
		cluster_count = clust32;
		fat_length = fatlength32;
		bs.fat_length = cpu_to_le16(0);
		bs.fat32.fat32_length = cpu_to_le32(fatlength32);
		
		memcpy(vi->fs_type, MSDOS_FAT32_SIGN, 8);
		root_dir_entries = 0;
		break;

	default:
		BUG();
	}

	/* Adjust the number of root directory entries to help enforce alignment */
	if (align_structures) {
		root_dir_entries = align_object(root_dir_sectors, bs.cluster_size) * (sector_size >> 5);
	}
	bs.sector_size[0] = (char)(sector_size & 0x00ff);
	bs.sector_size[1] = (char)((sector_size & 0xff00) >> 8);

	bs.dir_entries[0] = (char)(root_dir_entries & 0x00ff);
	bs.dir_entries[1] = (char)((root_dir_entries & 0xff00) >> 8);

	if (size_fat == 32) {
		/* set up additional FAT32 fields */
		bs.fat32.flags = cpu_to_le16(0);
		bs.fat32.version[0] = 0;
		bs.fat32.version[1] = 0;
		bs.fat32.root_cluster = cpu_to_le32(2);
		bs.fat32.info_sector = cpu_to_le16(1);
		if (!backup_boot)
			backup_boot = (reserved_sectors >= 7) ? 6 : (reserved_sectors >= 2) ? reserved_sectors - 1 : 0;
		else {
			if (backup_boot == 1)
				BUG();
			else if (backup_boot >= reserved_sectors)
				BUG();
		}
		if (verbose)
			printk("Using sector %d as backup boot sector (0 = none)\n", backup_boot);
		bs.fat32.backup_boot = cpu_to_le16(backup_boot);
		memset(&bs.fat32.reserved2, 0, sizeof(bs.fat32.reserved2));
	}
	if (num_sectors >= 65536) {
		bs.sectors[0] = (char)0;
		bs.sectors[1] = (char)0;
		bs.total_sect = cpu_to_le32(num_sectors);
	} else {
		bs.sectors[0] = (char)(num_sectors & 0x00ff);
		bs.sectors[1] = (char)((num_sectors & 0xff00) >> 8);
		bs.total_sect = cpu_to_le32(0);
	}
	vi->ext_boot_sign = MSDOS_EXT_SIGN;

	if (!cluster_count) {
		BUG();
	}

	/* The two following vars are in hard sectors, i.e. 512 byte sectors! */
	start_data_sector = (reserved_sectors + nr_fats * fat_length) * (sector_size / HARD_SECTOR_SIZE);
	start_data_block = (start_data_sector + SECTORS_PER_BLOCK - 1) / SECTORS_PER_BLOCK;

	if (blocks < start_data_block + 32)	/* Arbitrary undersize filesystem! */
		BUG();
	if (verbose) {
		printk("%s has %d head%s and %d sector%s per track,\n", device_name, le16_to_cpu(bs.heads),
	       (le16_to_cpu(bs.heads) != 1) ? "s" : "", le16_to_cpu(bs.secs_track),
	       (le16_to_cpu(bs.secs_track) != 1) ? "s" : "");
		printk("hidden sectors 0x%04x;\n",  hidden_sectors);
		printk("logical sector size is %d,\n", sector_size);
		printk("using 0x%02x media descriptor, with %d sectors;\n",(int)(bs.media), num_sectors);
		printk("drive number 0x%02x;\n", (int) (vi->drive_number));
		printk("filesystem has %d %d-bit FAT%s and %d sector%s per cluster.\n",
	       (int)(bs.fats), size_fat, (bs.fats != 1) ? "s" : "",
	       (int)(bs.cluster_size), (bs.cluster_size != 1) ? "s" : "");
		printk("FAT size is %d sector%s, and provides %d cluster%s.\n",
	       fat_length, (fat_length != 1) ? "s" : "",
	       cluster_count, (cluster_count != 1) ? "s" : "");
		printk("There %s %u reserved sector%s.\n",
	       (reserved_sectors != 1) ? "are" : "is",
	       reserved_sectors, (reserved_sectors != 1) ? "s" : "");

		if (size_fat != 32) {
			unsigned root_dir_entries = bs.dir_entries[0] + ((bs.dir_entries[1]) * 256);
			unsigned root_dir_sectors = cdiv(root_dir_entries * 32, sector_size);
	  	printk("Root directory contains %u slots and uses %u sectors.\n", root_dir_entries, root_dir_sectors);
		}
		printk("Volume ID is %08lx, ", volume_id & 0xffffffff);
		if (strcmp(volume_name, NO_NAME))
	    printk("volume label %s.\n", volume_name);
		else
	    printk("no volume label.\n");
	}
	/* Make the file allocation tables! */
	if (malloc_entire_fat)
		alloced_fat_length = fat_length;
	else
		alloced_fat_length = 1;

	if ((fat = (unsigned char *)malloc(alloced_fat_length * sector_size)) == NULL)
		BUG();

	memset(fat, 0, alloced_fat_length * sector_size);

	mark_FAT_cluster(0, 0xffffffff);	/* Initial fat entries */
	mark_FAT_cluster(1, 0xffffffff);
	fat[0] = (unsigned char)bs.media;	/* Put media type in first byte! */
	if (size_fat == 32) {
		/* Mark cluster 2 as EOF (used for root dir) */
		mark_FAT_cluster(2, FAT_EOF);
	}

	/* Make the root directory entries */
	size_root_dir = (size_fat == 32) ? bs.cluster_size * sector_size : 
					(((int)bs.dir_entries[1] * 256 + (int)bs.dir_entries[0]) 
					* sizeof(struct msdos_dir_entry));
	if ((root_dir = (struct msdos_dir_entry *)malloc(size_root_dir)) == NULL) {
		free(fat);		/* Tidy up before we die! */
		BUG();
	}
	memset(root_dir, 0, size_root_dir);
	if (memcmp(volume_name, NO_NAME, 11)) {
		struct msdos_dir_entry *de = &root_dir[0];
		memcpy(de->name, volume_name, 8);
		memcpy(de->ext, volume_name + 8, 3);
		de->attr = ATTR_VOLUME;
		//ctime = localtime(&create_time);
		memset(&ctime, sizeof(ctime), 0); 
		de->time = cpu_to_le16((unsigned short)((ctime.tm_sec >> 1) +
					    (ctime.tm_min << 5) +
					    (ctime.tm_hour << 11)));
		de->date = cpu_to_le16((unsigned short)(ctime.tm_mday +
				     	((ctime.tm_mon + 1) << 5) +
				     	((ctime.tm_year - 80) << 9)));
		de->ctime_ms = 0;
		de->ctime = de->time;
		de->cdate = de->date;
		de->adate = de->date;
		de->starthi = cpu_to_le16(0);
		de->start = cpu_to_le16(0);
		de->size = cpu_to_le32(0);
	}
	if (size_fat == 32) {
		/* For FAT32, create an info sector */
		struct fat32_fsinfo *info;

		if (!(info_sector = malloc(sector_size)))
			BUG();
		memset(info_sector, 0, sector_size);
		/* fsinfo structure is at offset 0x1e0 in info sector by observation */
		info = (struct fat32_fsinfo *)(info_sector + 0x1e0);

		/* Info sector magic */
		info_sector[0] = 'R';
		info_sector[1] = 'R';
		info_sector[2] = 'a';
		info_sector[3] = 'A';

		/* Magic for fsinfo structure */
		info->signature = cpu_to_le32(0x61417272);
		/* We've allocated cluster 2 for the root dir. */
		info->free_clusters = cpu_to_le32(cluster_count - 1);
		info->next_cluster = cpu_to_le32(2);

		/* Info sector also must have boot sign */
		*(unsigned short *) (info_sector + 0x1fe) = cpu_to_le16(BOOT_SIGN);
	}
	if (!(blank_sector = malloc(sector_size)))
		BUG();
	memset(blank_sector, 0, sector_size);
}

/* Write the new filesystem's data tables to wherever they're going to end up! */

static void write_tables(void)
{
	int x;
	int fat_length;
	int write_pos = 0;

	fat_length = (size_fat == 32) ? le32_to_cpu(bs.fat32.fat32_length) : le16_to_cpu(bs.fat_length);

  write_pos = 0;
  /* clear all reserved sectors */
  for (x = 0; x < reserved_sectors; ++x)
  {
  	memcpy(root_ramdisk_addr + write_pos, blank_sector, sector_size);
	write_pos += sector_size;
  	}
		
  /* seek back to sector 0 and write the boot sector */
  write_pos = 0;

  memcpy(root_ramdisk_addr + write_pos, &bs, sizeof(struct msdos_boot_sector));
  write_pos += sizeof(struct msdos_boot_sector);
  
  /* on FAT32, write the info sector and backup boot sector */
  if (size_fat == 32) {
  		write_pos = le16_to_cpu(bs.fat32.info_sector) * sector_size;
		memcpy(root_ramdisk_addr + write_pos, info_sector, 512);
		write_pos += 512;
		if (backup_boot != 0) {
	  		write_pos = backup_boot * sector_size;
			memcpy(root_ramdisk_addr + write_pos, &bs, sizeof(struct msdos_boot_sector));
			write_pos += sizeof(struct msdos_boot_sector);
		}
  }
  
  /* seek to start of FATS and write them all */
  write_pos = reserved_sectors * sector_size;
  for (x = 1; x <= nr_fats; x++) {
		int y;
		int blank_fat_length = fat_length - alloced_fat_length;
		memcpy(root_ramdisk_addr + write_pos, fat, alloced_fat_length * sector_size);
		write_pos += alloced_fat_length * sector_size;
		for (y = 0; y < blank_fat_length; y++)
		{
			memcpy(root_ramdisk_addr + write_pos, blank_sector, sector_size);
			write_pos += sector_size;
		}
  }
  
  /* Write the root directory directly after the last FAT. This is the root
  * dir area on FAT12/16, and the first cluster on FAT32. */
  memcpy(root_ramdisk_addr + write_pos, (char *)root_dir, size_root_dir);
  write_pos += size_root_dir;

  if (blank_sector)
		free(blank_sector);
  if (info_sector)
		free(info_sector);
  free(root_dir);		/* Free up the root directory space from setup_tables */
  free(fat);			/* Free up the fat table space reserved during setup_tables */
}


int FormatRamWithFAT(char	*rd_start, int rd_length)
{	
	static struct timeval create_timeval;	/* Creation time */
	create_timeval.tv_sec = 0;
	create_timeval.tv_usec = 0;
	volume_id = (u_int32_t) ((create_timeval.tv_sec << 20) | create_timeval.tv_usec);

	volume_name = "dim-sum rd";
	sector_size = 512;
	size_fat = 12;
	size_fat_by_user = 1;
	blocks = root_ramdisk_size / BLOCK_SIZE;
	root_ramdisk_addr = rd_start;//AllocPermanentBootMemory(BOOT_MEMORY_MODULE_UNKNOWN, root_ramdisk_size, 0);
	root_ramdisk_size = rd_length;
	
	memset(root_ramdisk_addr, 0, root_ramdisk_size);
	
	establish_params();
	setup_tables();
	write_tables();

	return 0;
}