#include <base/string.h>
#include <linux/time.h>
#include <dim-sum/mem.h>
#include <fs/fs.h>
#include <fs/msdos_fs.h>
#include <linux/time.h>
#include <dim-sum/sched.h>

#include "msbuffer.h"

#define PRINTK(x)
#define Printk(x)	printk x
/* Well-known binary file extensions */

static char bin_extensions[] =
  "EXECOMBINAPPSYSDRVOVLOVROBJLIBDLLPIF"	/* program code */
  "ARCZIPLHALZHZOOTARZ  ARJ"	/* common archivers */
  "TZ TAZTZPTPZ"		/* abbreviations of tar.Z and tar.zip */
  "GZ TGZDEB"			/* .gz, .tar.gz and Debian packages   */
  "GIFBMPTIFGL JPGPCX"		/* graphics */
  "TFMVF GF PK PXLDVI";		/* TeX */


/*
 * fs_panic reports a severe file system problem and sets the file system
 * read-only. The file system can be made writable again by remounting it.
 */

void fs_panic(struct super_block *s,char *msg)
{
	int not_ro;

	not_ro = !(s->s_flags & MS_RDONLY);
	if (not_ro) s->s_flags |= MS_RDONLY;
	printk("Filesystem panic (dev 0x%04X, mounted on 0x%04X:%ld)\n  %s\n",
	    s->s_dev,s->s_covered->i_dev,s->s_covered->i_ino,msg);
	if (not_ro)
		printk("  File system has been set read-only\n");
}


/*
 * is_binary selects optional text conversion based on the conversion mode and
 * the extension part of the file name.
 */

int is_binary(char conversion,char *extension)
{
	char *walk;

	switch (conversion) {
		case 'b':
			return 1;
		case 't':
			return 0;
		case 'a':
			for (walk = bin_extensions; *walk; walk += 3)
				if (!strncmp(extension,walk,3)) return 1;
			return 0;
		default:
			printk("Invalid conversion mode - defaulting to "
			    "binary.\n");
			return 1;
	}
}


/* File creation lock. This is system-wide to avoid deadlocks in rename. */
/* (rename might deadlock before detecting cross-FS moves.) */

static DECLARE_WAIT_QUEUE_HEAD(creation_wait);
static int creation_lock = 0;


void lock_creation(void)
{
	while (creation_lock) sleep_on(&creation_wait);
	creation_lock = 1;
}


void unlock_creation(void)
{
	creation_lock = 0;
	wake_up(&creation_wait);
}


void lock_fat(struct super_block *sb)
{
	while (MSDOS_SB(sb)->fat_lock) sleep_on(&MSDOS_SB(sb)->fat_wait);
	MSDOS_SB(sb)->fat_lock = 1;
}


void unlock_fat(struct super_block *sb)
{
	MSDOS_SB(sb)->fat_lock = 0;
	wake_up(&MSDOS_SB(sb)->fat_wait);
}


/*
 * msdos_add_cluster tries to allocate a new cluster and adds it to the file
 * represented by inode. The cluster is zero-initialized.
 */

int msdos_add_cluster(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	int count,nr,limit,last,cur,sector,last_sector,file_cluster;
	struct buffer_head *bh;
	int cluster_size = MSDOS_SB(inode->i_sb)->cluster_size;

	if (inode->i_ino == MSDOS_ROOT_INO) return -ENOSPC;
	if (!MSDOS_SB(inode->i_sb)->free_clusters) return -ENOSPC;
	lock_fat(inode->i_sb);
	limit = MSDOS_SB(inode->i_sb)->clusters;
	nr = limit; /* to keep GCC happy */
	for (count = 0; count < limit; count++) {
		nr = ((count+MSDOS_SB(inode->i_sb)->prev_free) % limit)+2;
		if (fat_access(inode->i_sb,nr,-1) == 0) break;
	}
	PRINTK (("cnt = %d --",count));
#ifdef DEBUG
printk("free cluster: %d\n",nr);
#endif
	MSDOS_SB(inode->i_sb)->prev_free = (count+MSDOS_SB(inode->i_sb)->
	    prev_free+1) % limit;
	if (count >= limit) {
		MSDOS_SB(inode->i_sb)->free_clusters = 0;
		unlock_fat(inode->i_sb);
		return -ENOSPC;
	}
	fat_access(inode->i_sb,nr,MSDOS_SB(inode->i_sb)->fat_bits == 12 ?
	    0xff8 : 0xfff8);
	if (MSDOS_SB(inode->i_sb)->free_clusters != -1)
		MSDOS_SB(inode->i_sb)->free_clusters--;
	unlock_fat(inode->i_sb);
#ifdef DEBUG
printk("set to %x\n",fat_access(inode->i_sb,nr,-1));
#endif
	last = 0;
	/* We must locate the last cluster of the file to add this
	   new one (nr) to the end of the link list (the FAT).
	   
	   Here file_cluster will be the number of the last cluster of the
	   file (before we add nr).
	   
	   last is the corresponding cluster number on the disk. We will
	   use last to plug the nr cluster. We will use file_cluster to
	   update the cache.
	*/
	file_cluster = 0;
	if ((cur = MSDOS_I(inode)->i_start) != 0) {
		cache_lookup(inode,INT_MAX,&last,&cur);
		file_cluster = last;
		while (cur && cur != -1){
			PRINTK (("."));
			file_cluster++;
			if (!(cur = fat_access(inode->i_sb,
			    last = cur,-1))) {
				fs_panic(inode->i_sb,"File without EOF");
				return -ENOSPC;
			}
		}
		PRINTK ((" --  "));
	}
#ifdef DEBUG
printk("last = %d\n",last);
#endif
	if (last) fat_access(inode->i_sb,last,nr);
	else {
		MSDOS_I(inode)->i_start = nr;
		inode->i_dirt = 1;
	}
#ifdef DEBUG
if (last) printk("next set to %d\n",fat_access(inode->i_sb,last,-1));
#endif
	sector = MSDOS_SB(inode->i_sb)->data_start+(nr-2)*cluster_size;
	last_sector = sector + cluster_size;
	for ( ; sector < last_sector; sector++) {
		#ifdef DEBUG
			printk("zeroing sector %d\n",sector);
		#endif
		if (!(bh = getblk(inode->i_dev,sector,SECTOR_SIZE)))
			printk("getblk failed\n");
		else {
			memset(bh->b_data,0,SECTOR_SIZE);
			msdos_set_uptodate(sb,bh,1);
			mark_buffer_dirty(bh, 1);
			brelse(bh);
		}
	}
	if (file_cluster != inode->i_blocks/cluster_size){
		printk ("file_cluster badly computed!!! %d <> %ld\n"
			,file_cluster,inode->i_blocks/cluster_size);
	}else{
		cache_add(inode,file_cluster,nr);
	}
	inode->i_blocks += cluster_size;
	if (S_ISDIR(inode->i_mode)) {
		if (inode->i_size & (SECTOR_SIZE-1)) {
			fs_panic(inode->i_sb,"Odd directory size");
			inode->i_size = (inode->i_size+SECTOR_SIZE) &
			    ~(SECTOR_SIZE-1);
		}
		inode->i_size += SECTOR_SIZE*cluster_size;
#ifdef DEBUG
printk("size is %lu now (%p)\n",inode->i_size,inode);
#endif
		inode->i_dirt = 1;
	}
	return 0;
}


/* Linear day numbers of the respective 1sts in non-leap years. */

static int day_n[] = { 0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 };
		  /* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */


extern struct timezone sys_tz;

struct timezone sys_tz;

/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). */

int date_dos2unix(unsigned short time,unsigned short date)
{
	int month,year,secs;

	month = ((date >> 5) & 15)-1;
	year = date >> 9;
	secs = (time & 31)*2+60*((time >> 5) & 63)+(time >> 11)*3600+86400*
	    ((date & 31)-1+day_n[month]+(year/4)+year*365-((year & 3) == 0 &&
	    month < 2 ? 1 : 0)+3653);
			/* days since 1.1.70 plus 80's leap day */
	secs += sys_tz.tz_minuteswest*60;
	return secs;
}


/* Convert linear UNIX date to a MS-DOS time/date pair. */

void date_unix2dos(int unix_date,unsigned short *time,
    unsigned short *date)
{
	int day,year,nl_day,month;

	unix_date -= sys_tz.tz_minuteswest*60;
	*time = (unix_date % 60)/2+(((unix_date/60) % 60) << 5)+
	    (((unix_date/3600) % 24) << 11);
	day = unix_date/86400-3652;
	year = day/365;
	if ((year+3)/4+365*year > day) year--;
	day -= (year+3)/4+365*year;
	if (day == 59 && !(year & 3)) {
		nl_day = day;
		month = 2;
	}
	else {
		nl_day = (year & 3) || day <= 59 ? day : day-1;
		for (month = 0; month < 12; month++)
			if (day_n[month] > nl_day) break;
	}
	*date = nl_day-day_n[month-1]+1+(month << 5)+(year << 9);
}


/* Returns the inode number of the directory entry at offset pos. If bh is
   non-NULL, it is brelse'd before. Pos is incremented. The buffer header is
   returned in bh. */

int msdos_get_entry(struct inode *dir, loff_t *pos,struct buffer_head **bh,
    struct msdos_dir_entry **de)
{
	struct super_block *sb = dir->i_sb;
	int sector,offset;

	while (1) {
		offset = *pos;
		PRINTK (("get_entry offset %d\n",offset));
		if ((sector = msdos_smap(dir,offset >> SECTOR_BITS)) == -1)
			return -1;
		PRINTK (("get_entry sector %d %p\n",sector,*bh));
		if (!sector)
			return -1; /* beyond EOF */
		*pos += sizeof(struct msdos_dir_entry);
		if (*bh)
			brelse(*bh);
		PRINTK (("get_entry sector apres brelse\n"));
		if (!(*bh = bread(dir->i_dev,sector,SECTOR_SIZE))) {
			printk("Directory sread (sector %d) failed\n",sector);
			continue;
		}
		PRINTK (("get_entry apres sread\n"));
		*de = (struct msdos_dir_entry *) ((*bh)->b_data+(offset &
		    (SECTOR_SIZE-1)));
		return (sector << MSDOS_DPS_BITS)+((offset & (SECTOR_SIZE-1)) >>
		    MSDOS_DIR_BITS);
	}
}


/*
 * Now an ugly part: this set of directory scan routines works on clusters
 * rather than on inodes and sectors. They are necessary to locate the '..'
 * directory "inode". raw_scan_sector operates in four modes:
 *
 * name     number   ino      action
 * -------- -------- -------- -------------------------------------------------
 * non-NULL -        X        Find an entry with that name
 * NULL     non-NULL non-NULL Find an entry whose data starts at *number
 * NULL     non-NULL NULL     Count subdirectories in *number. (*)
 * NULL     NULL     non-NULL Find an empty entry
 *
 * (*) The return code should be ignored. It DOES NOT indicate success or
 *     failure. *number has to be initialized to zero.
 *
 * - = not used, X = a value is returned unless NULL
 *
 * If res_bh is non-NULL, the buffer is not deallocated but returned to the
 * caller on success. res_de is set accordingly.
 *
 * If cont is non-zero, raw_found continues with the entry after the one
 * res_bh/res_de point to.
 */


static int raw_scan_sector(struct super_block *sb,int sector,char *name,
    int *number,int *ino,struct buffer_head **res_bh,
    struct msdos_dir_entry **res_de)
{
	struct buffer_head *bh;
	struct msdos_dir_entry *data;
	struct inode *inode;
	int entry,start,done;

	if (!(bh = bread(sb->s_dev,sector,SECTOR_SIZE))) return -EIO;
	data = (struct msdos_dir_entry *) bh->b_data;
	for (entry = 0; entry < MSDOS_DPS; entry++) {
		if (name) {
			done = !strncmp(data[entry].name,name,MSDOS_NAME) 
				&& !(data[entry].attr & ATTR_VOLUME);
		}
		else {
			if (!ino) {
				done = 0; 
				if (!IS_FREE(data[entry].name) && (data[entry].attr & ATTR_DIR)) {
	    			(*number)++; 
				}		
			}
			else {
				if (number) {
					done = !IS_FREE(data[entry].name) && CF_LE_W(data[entry].start) == *number;
				}
				else {
					done = IS_FREE(data[entry].name); 
					if (done) { 
	    				inode = iget(sb,sector*MSDOS_DPS+entry); 
	    				if (inode) { 
	    					/* Directory slots of busy deleted files aren't available yet. */ 
							done = !MSDOS_I(inode)->i_busy; 
							iput(inode); 
	    				} 
					} 
				}
			}
		}
		if (done) {
			if (ino) *ino = sector*MSDOS_DPS+entry;
			start = CF_LE_W(data[entry].start);
			if (!res_bh) brelse(bh);
			else {
				*res_bh = bh;
				*res_de = &data[entry];
			}
			return start;
		}
	}
	brelse(bh);
	return -ENOENT;
}


/*
 * raw_scan_root performs raw_scan_sector on the root directory until the
 * requested entry is found or the end of the directory is reached.
 */

static int raw_scan_root(struct super_block *sb,char *name,int *number,int *ino,
    struct buffer_head **res_bh,struct msdos_dir_entry **res_de)
{
	int count,cluster;

	
	for (count = 0; count < MSDOS_SB(sb)->dir_entries/MSDOS_DPS; count++) {
		if ((cluster = raw_scan_sector(sb,MSDOS_SB(sb)->dir_start+count,
		    name,number,ino,res_bh,res_de)) >= 0) {
		    return cluster;
		}
		else{
			//printk_xby("xby_debug in raw_scan_root, cluster is %d\n", cluster);
		}
	}

	return -ENOENT;
}


/*
 * raw_scan_nonroot performs raw_scan_sector on a non-root directory until the
 * requested entry is found or the end of the directory is reached.
 */

static int raw_scan_nonroot(struct super_block *sb,int start,char *name,
    int *number,int *ino,struct buffer_head **res_bh,struct msdos_dir_entry
    **res_de)
{
	int count,cluster;

#ifdef DEBUG
	printk("raw_scan_nonroot: start=%d\n",start);
#endif
	do {
		for (count = 0; count < MSDOS_SB(sb)->cluster_size; count++) {
			if ((cluster = raw_scan_sector(sb,(start-2)*
			    MSDOS_SB(sb)->cluster_size+MSDOS_SB(sb)->data_start+
			    count,name,number,ino,res_bh,res_de)) >= 0)
				return cluster;
		}
		if (!(start = fat_access(sb,start,-1))) {
			fs_panic(sb,"FAT error");
			break;
		}
#ifdef DEBUG
	printk("next start: %d\n",start);
#endif
	}
	while (start != -1);
	return -ENOENT;
}


/*
 * raw_scan performs raw_scan_sector on any sector.
 *
 * NOTE: raw_scan must not be used on a directory that is is the process of
 *       being created.
 */

static int raw_scan(struct super_block *sb,int start,char *name,int *number,
    int *ino,struct buffer_head **res_bh,struct msdos_dir_entry **res_de)
{
	if (start)
		return raw_scan_nonroot(sb,start,name,number,ino,res_bh,res_de);
	else return raw_scan_root(sb,name,number,ino,res_bh,res_de);
}


/*
 * msdos_parent_ino returns the inode number of the parent directory of dir.
 * File creation has to be deferred while msdos_parent_ino is running to
 * prevent renames.
 */

int msdos_parent_ino(struct inode *dir,int locked)
{
	static int zero = 0;
	int error,cur,prev,nr;

	if (!S_ISDIR(dir->i_mode)) panic("Non-directory fed to m_p_i");
	if (dir->i_ino == MSDOS_ROOT_INO) return dir->i_ino;
	if (!locked) lock_creation(); /* prevent renames */
	if ((cur = raw_scan(dir->i_sb,MSDOS_I(dir)->i_start,MSDOS_DOTDOT,
	    &zero,NULL,NULL,NULL)) < 0) {
		if (!locked) unlock_creation();
		return cur;
	}
	if (!cur) nr = MSDOS_ROOT_INO;
	else {
		if ((prev = raw_scan(dir->i_sb,cur,MSDOS_DOTDOT,&zero,NULL,
		    NULL,NULL)) < 0) {
			if (!locked) unlock_creation();
			return prev;
		}
		if ((error = raw_scan(dir->i_sb,prev,NULL,&cur,&nr,NULL,
		    NULL)) < 0) {
			if (!locked) unlock_creation();
			return error;
		}
	}
	if (!locked) unlock_creation();
	return nr;
}


/*
 * msdos_subdirs counts the number of sub-directories of dir. It can be run
 * on directories being created.
 */

int msdos_subdirs(struct inode *dir)
{
	int count;

	count = 0;
	if (dir->i_ino == MSDOS_ROOT_INO)
		(void) raw_scan_root(dir->i_sb,NULL,&count,NULL,NULL,NULL);
	else {
		if (!MSDOS_I(dir)->i_start) return 0; /* in mkdir */
		else (void) raw_scan_nonroot(dir->i_sb,MSDOS_I(dir)->i_start,
		    NULL,&count,NULL,NULL,NULL);
	}
	return count;
}


/*
 * Scans a directory for a given file (name points to its formatted name) or
 * for an empty directory slot (name is NULL). Returns an error code or zero.
 */

int msdos_scan(struct inode *dir,char *name,struct buffer_head **res_bh,
    struct msdos_dir_entry **res_de,int *ino)
{
	int res;

	if (name)
		res = raw_scan(dir->i_sb,MSDOS_I(dir)->i_start,name,NULL,ino,
		    res_bh,res_de);
	else res = raw_scan(dir->i_sb,MSDOS_I(dir)->i_start,NULL,NULL,ino,
		    res_bh,res_de);
	return res < 0 ? res : 0;
}
