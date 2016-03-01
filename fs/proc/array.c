#include <base/string.h>
#include <dim-sum/timer.h>
#include <fs/fs.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <fs/proc_fs.h>
#include <linux/time.h>
#include <asm/errno.h>
#include <dim-sum/mem.h>

#define FSHIFT		11		/* nr of bits of precision */
#define FIXED_1		(1<<FSHIFT)	/* 1.0 as fixed-point */

#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)

/**
 * 用于演示目的，请修改
 */
static int get_loadavg(char * buffer)
{
	int a, b, c;

	a = 19;
	b = 76;
	c = 12;
	return sprintf(buffer,"%d.%02d %d.%02d %d.%02d %d/%d\n",
		LOAD_INT(a), LOAD_FRAC(a),
		LOAD_INT(b), LOAD_FRAC(b),
		LOAD_INT(c), LOAD_FRAC(c),
		2, 10);
}



static int get_uptime(char * buffer)
{
	unsigned long uptime;
	unsigned long idle;

	uptime = get_jiffies_64();
	idle = 123;

	/* The formula for the fraction parts really is ((t * 100) / HZ) % 100, but
	   that would overflow about every five days at HZ == 100.
	   Therefore the identity a = (a / b) * b + a % b is used so that it is
	   calculated as (((t / HZ) * 100) + ((t % HZ) * 100) / HZ) % 100.
	   The part in front of the '+' always evaluates as 0 (mod 100). All divisions
	   in the above formulas are truncating. For HZ being a power of 10, the
	   calculations simplify to the version in the #else part (if the printf
	   format is adapted to the same number of digits as zeroes in HZ.
	 */
	return sprintf(buffer,"%lu.%02lu %lu.%02lu\n",
		uptime / HZ,
		uptime % HZ,
		idle / HZ,
		idle % HZ);
}

static int get_none(char * buffer)
{
	return sprintf(buffer,"have not done\n");
}

static int get_meminfo(char * buffer)
{
	return get_none(buffer);
}
static int get_version(char * buffer)
{
	return get_none(buffer);
}
static int get_cpuinfo(char * buffer)
{
	return get_none(buffer);
}
static int get_irq_list(char * buffer)
{
	return get_none(buffer);
}

static int get_root_array(char * page, int type)
{
	switch (type) {
		case PROC_LOADAVG:
			return get_loadavg(page);

		case PROC_UPTIME:
			return get_uptime(page);

		case PROC_MEMINFO:
			return get_meminfo(page);
			
		case PROC_CPUINFO:
			return get_cpuinfo(page);

		case PROC_VERSION:
			return get_version(page);

		case PROC_INTERRUPTS:
			return get_irq_list(page);
	}
	return -EBADF;
}

static inline int fill_array(char * page, int pid, int type)
{
	return get_root_array(page, type);
}

static int array_read(struct inode * inode, struct file * file,char * buf, int count)
{
	unsigned long page;
	int length;
	int end;
	unsigned int type, pid;

	if (count < 0)
		return -EINVAL;
	if (!(page = (unsigned long)dim_sum_pages_alloc(0, MEM_NORMAL)))
		return -ENOMEM;
	type = inode->i_ino;
	pid = type >> 16;
	type &= 0x0000ffff;
	length = fill_array((char *) page, pid, type);
	if (length < 0) {
		dim_sum_pages_free((void *)page);
		return length;
	}
	if (file->f_pos >= length) {
		dim_sum_pages_free((void *)page);
		return 0;
	}
	if (count + file->f_pos > length)
		count = length - file->f_pos;
	end = count + file->f_pos;
	memcpy_tofs(buf, (char *) page + file->f_pos, count);
	dim_sum_pages_free((void *)page);
	file->f_pos = end;
	return count;
}


int proc_match(int len,const char * name,struct proc_dir_entry * de)
{
	if (!de || !de->low_ino)
		return 0;
	/* "" means "." ---> so paths like "/usr/lib//libc.a" work */
	if (!len && (de->name[0]=='.') && (de->name[1]=='\0'))
		return 1;
	if (de->namelen != len)
		return 0;
	return !memcmp(name, de->name, len);
}


static struct file_operations proc_array_operations = {
	NULL,		/* array_lseek */
	array_read,
	NULL,		/* array_write */
	NULL,		/* array_readdir */
	NULL,		/* array_select */
	NULL,		/* array_ioctl */
	NULL,		/* mmap */
	NULL,		/* no special open code */
	NULL,		/* no special release code */
	NULL		/* can't fsync */
};

struct inode_operations proc_array_inode_operations = {
	&proc_array_operations,	/* default base directory file-ops */
	NULL,			/* create */
	NULL,			/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* bmap */
	NULL,			/* truncate */
	NULL			/* permission */
};

