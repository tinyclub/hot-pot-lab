#include <asm/unistd.h>
//#include <sys/types.h>
//#include <sys/poll.h>
//#include <sys/socket.h>
//#include <sys/dirent.h>
//#include <bitsize.h>
#include <linux/resource.h>
#include <linux/capability.h>
#include <fs/vfs.h>
#include <dim-sum/syscall.h>
#include <dim-sum/irqflags.h>
#include <asm/asm-offsets.h>
#include <asm-generic/current.h>
#include <dim-sum/base.h>
#include <dim-sum/sched.h>
#include <base/string.h>
#include <asm-generic/siginfo.h>

/**
 * 桩函数
 */
int verify_area(int type, const void * addr, unsigned long size)
{
	return 0;
}

#define SAVE_REGS(size)	\
	asm("sub	sp, sp, %0@ Allocate frame size in one go" : : "i" (size));	\
	asm("stmia	sp, {r0 - lr}			@ Save XXX r0 - lr");
#define RESTORE_REGS(size)	\
	asm("add	sp, sp, %0@ Allocate frame size in one go" : : "i" (size));		

static inline void enter_syscall(void)
{
	raw_local_irq_disable();
	SAVE_REGS(S_FRAME_SIZE);
	barrier();
	if (!current->in_syscall)
	{
		register struct pt_regs *regs asm ("sp");
		current->uregs = regs;
		current->in_syscall = 1;
	}
	raw_local_irq_enable();
}

extern void svc_tail(struct pt_regs *regs);

static inline void exit_syscall(void)
{
	raw_local_irq_disable();
	
	current->in_syscall = 0;
	svc_tail(current->uregs);
	current->uregs = NULL;
	barrier();
	RESTORE_REGS(S_FRAME_SIZE);
	
	raw_local_irq_enable();
}

#define sys_call_and_return(type, func)	\
	do {	\
		type ret;				\
		enter_syscall();	\
		ret = func;			\
		exit_syscall();		\
		return ret;			\
	}while (0)

/**
 * 从用户空间复制任意大小的块。
 * 对dim-sum来说，直接复制即可。
 */
unsigned long
copy_from_user(void *to, const void *from, unsigned long n)
{
	//might_sleep();
	BUG_ON((long) n < 0);

	memcpy(to, from, (size_t)n);
	n = 0;

	return n;
}


/*
 * Process-related syscalls
 */
__noreturn void _exit(int a)
{
	sys_exit(a);
}

#ifdef xby_debug
pid_t __clone(unsigned long a, void *b)
{
	#sys_call_and_return sys_clone(a, b);
}

pid_t fork(void)
{
	#sys_call_and_return sys_fork();
}

pid_t vfork()
{
	#sys_call_and_return sys_vfork();
}
#endif


pid_t getpid(void)
{
	sys_call_and_return(pid_t, sys_getpid());
}

uid_t getuid(void)
{
	sys_call_and_return(pid_t, sys_getuid());
}

#ifdef xby_debug
int setpgid(pid_t a, pid_t b)
{
	#sys_call_and_return sys_setpgid(a, b);
}

pid_t getpgid(pid_t a)
{
	#sys_call_and_return sys_getpgid(a);
}

pid_t getppid(void)
{
	#sys_call_and_return sys_getppid();
}

pid_t setsid(void)
{
	#sys_call_and_return sys_setsid();
}

pid_t getsid(pid_t a)
{
	#sys_call_and_return sys_getsid(a);
}
#endif

pid_t wait4(pid_t a, int *b, int c, struct rusage *d)
{
	sys_call_and_return(pid_t, sys_wait4(a, b, c, d));
}

#ifdef xby_debug
int execve(const char *a, char * const *b, char * const *c)
{
	#sys_call_and_return sys_execve(a, b, c);
}

int nice(int a)
{
	#sys_call_and_return sys_nice(a);
}

int __getpriority(int a, int b)
{
	#sys_call_and_return sys_getpriority(a, b);
}

int setpriority(int a, int b, int c)
{
	#sys_call_and_return sys_getpriority(a, b, c);
}

int getrusage(int a, struct rusage *b)
{
	#sys_call_and_return sys_getrusage(a, b);
}

int sched_setscheduler(pid_t a, int b, const struct sched_param *c)
{
	#sys_call_and_return sys_sched_setscheduler(a, b, c);
}

int sched_setaffinity(pid_t a, unsigned int b, unsigned long *c)
{
	#sys_call_and_return sys_sched_setaffinity(a, b, c);
}

int sched_getaffinity(pid_t a, unsigned int b, unsigned long *c)
{
	#sys_call_and_return sys_sched_setaffinity(a, b, c);
}

int sched_yield(void)
{
	#sys_call_and_return sys_sched_yield();
}

int prctl(int a, unsigned long b, unsigned long c, unsigned long d, unsigned long e)
{
	#sys_call_and_return sys_prctl(a, b, c, d, e);
}

/*
 * User and group IDs
 */
int setuid(uid_t a)
{
	#sys_call_and_return sys_setuid(a);
}

int setgid(gid_t a)
{
	#sys_call_and_return sys_setgid(a);
}

uid_t getuid(void)
{
	#sys_call_and_return sys_getuid();
}

gid_t getgid(void)
{
	#sys_call_and_return sys_getgid();
}

uid_t geteuid(void)
{
	#sys_call_and_return sys_geteuid();
}

gid_t getegid(void)
{
	#sys_call_and_return sys_getegid();
}

int getgroups(int a, gid_t *b)
{
	#sys_call_and_return sys_getgroups(a, b);
}

int setgroups(size_t a, const gid_t *b)
{
	#sys_call_and_return sys_setgroups(a, b);
}

int setreuid(uid_t a, uid_t b)
{
	#sys_call_and_return sys_setreuid(a, b);
}

int setregid(gid_t a, gid_t b)
{
	#sys_call_and_return sys_setregid(a, b);
}

int setfsuid(uid_t a)
{
	#sys_call_and_return sys_setregid(a);
}
int setfsgid(gid_t a)
{
	#sys_call_and_return sys_setfsgid(a);
}

int setresuid(int a, uid_t b, uid_t c, uid_t d)
{
	#sys_call_and_return sys_setresuid(a, b, c, d);
}

/*
 * POSIX Capabilities
 */
int capget(cap_user_header_t a, cap_user_data_t b)
{
	#sys_call_and_return sys_capget(a, b);
}
int capset(cap_user_header_t a, cap_user_data_t b)
{
	#sys_call_and_return sys_capset(a, b);
}
#endif
/*
 * Filesystem-related system calls
 */
int mount(char *a, char *b, char *c, unsigned long d, const void *e)
{
	sys_call_and_return(int, sys_mount(a, b, c, d, e));
}

int umount2(const char *a, int b)
{
	sys_call_and_return(int, sys_umount(a));
}

#ifdef xby_debug
int pivot_root(const char *a, const char *b)
{
	#sys_call_and_return sys_prvot_root(a, b);
}
#endif

int sync(void)
{
	sys_call_and_return(int, sys_sync());
}

int statfs(const char *a, struct statfs *b)
{
	sys_call_and_return(int, sys_statfs(a, b));
}

int fstatfs(int a, struct statfs *b)
{
	sys_call_and_return(int, sys_fstatfs(a, b));
}

#ifdef xby_debug
int swapon(const char *a, int b)
{
	#sys_call_and_return sys_swapon(a, b);
}

int swapoff(const char *a)
{
	#sys_call_and_return sys_swapoff(a);
}
#endif

/*
 * Inode-related system calls
 */
int access(const char *a, int b)
{
	sys_call_and_return(int, sys_access(a, b));
}

#ifdef xby_debug
int faccessat(int a, const char *b, int c, int d)
{
	#sys_call_and_return sys_faccessat(a, b, c, d);
}
#endif

int link(const char * a, const char *b)
{
	sys_call_and_return(int, sys_link(a, b));
}

#ifdef xby_debug
int linkat(int a, const char *b, int c, const char *d)
{
	#sys_call_and_return sys_linkat(a, b, c, d);
}
#endif

int unlink(const char *a)
{
	sys_call_and_return(int, sys_unlink(a));
}

#ifdef xby_debug
int unlinkat(int a, const char *b, int c)
{
	#sys_call_and_return sys_unlinkat(a, b, c);
}
#endif

int chdir(const char *a)
{
	sys_call_and_return(int, sys_chdir(a));
}

int fchdir(int a)
{
	sys_call_and_return(int, sys_fchdir(a));
}

int rename(const char *a, const char *b)
{
	sys_call_and_return(int, sys_rename(a, b));
}

#ifdef xby_debug
int renameat(int a, const char *b, int c, const char *d)
{
	#sys_call_and_return sys_renameat(a, b, c, d);
}
#endif

int mknod(const char *a, mode_t b, dev_t c)
{
	sys_call_and_return(int, sys_mknod(a, b, c));
}

#ifdef xby_debug
int mknodat(int a, const char *b, const char *c, mode_t d, dev_t e)
{
	#sys_call_and_return sys_mknodat(a, b, c, d, e);
}
#endif

int chmod(const char *a, mode_t b)
{
	sys_call_and_return(int, sys_chmod(a, b));
}

int fchmod(int a, mode_t b)
{
	sys_call_and_return(int, sys_fchmod(a, b));
}

#ifdef xby_debug
int fchmodat(int a, const char *b, mode_t c)
{
	#sys_call_and_return sys_fchmodat(a, b, c);
}
#endif

int mkdir(const char *a, mode_t b)
{
	sys_call_and_return(int, sys_mkdir(a, b));
}

#ifdef xby_debug
int mkdirat(int a, const char *b, const char *c, mode_t d)
{
	#sys_call_and_return sys_mkdirat(a, b, c, d);
}
#endif

int rmdir(const char *a)
{
	sys_call_and_return(int, sys_rmdir(a));
}

int pipe(int *a)
{
	sys_call_and_return(int, sys_pipe(a));
}


#ifdef xby_debug
int pipe2(int *a, int b)
{
	#sys_call_and_return sys_pipe2(a, b);
}
#endif

mode_t umask(mode_t a)
{
	sys_call_and_return(mode_t, sys_umask(a));
}


int chroot(const char *a)
{
	sys_call_and_return(int, sys_chroot(a));
}

int symlink(const char *a, const char *b)
{
	sys_call_and_return(int, sys_symlink(a, b));
}

#ifdef xby_debug
int symlinkat(const char *a, int b, const char *c)
{
	#sys_call_and_return sys_symlinkat(a, b, c);
}
#endif

int readlink(const char *a, char *b, size_t c)
{
	sys_call_and_return(int, sys_readlink(a, b, c));
}

#ifdef xby_debug
int readlinkat(int a, const char *b, char *c, int d)
{
	#sys_call_and_return sys_readlinkat(a, b, c, d);
}
#endif

int stat(const char *a, struct stat *b)
{
	sys_call_and_return(int, sys_stat(a, b));
}

int lstat(const char *a, struct stat *b)
{
	sys_call_and_return(int, sys_lstat(a, b));
}

int fstat(int a, struct stat *b)
{
	sys_call_and_return(int, sys_fstat(a, b));
}

/* XXX: Is this right?! */
#ifdef xby_debug
int fstatat(int a, const char *b, struct stat *c, int d)
{
	#sys_call_and_return sys_fstatat(a, b, c, d);
}
#endif

int getdents(unsigned int a, struct dirent *b, unsigned int c)
{
	sys_call_and_return(int, sys_readdir(a, b, c));
}


int chown(const char *a, uid_t b, gid_t c)
{
	sys_call_and_return(int, sys_chown(a, b, c));
}

int fchown(int a, uid_t b, gid_t c)
{
	sys_call_and_return(int, sys_fchown(a, b, c));
}

#ifdef xby_debug
int fchownat(int a, const char *b, uid_t c, gid_t d, int e)
{
	#sys_call_and_return sys_fchownat(a, b, c, d, e);
}

int lchown(const char * a, uid_t b, gid_t c)
{
	#sys_call_and_return sys_lchown(a, b, c);
}
#endif

int __getcwd(char *a, size_t b)
{
	return -38;
	//xby_debug
	//return sys_getcwd(a, b);
}

int utime(const char *a, const struct utimbuf *b)
{
	sys_call_and_return(int, sys_utime(a, b));
}

#ifdef xby_debug
int utimes(const char *a, const struct timeval *b)
{
	#sys_call_and_return sys_utimes(a, b);
}

int futimesat(int a, const char *b, const struct timeval *c)
{
	#sys_call_and_return sys_futimesat(a, b, c);
}

int inotify_init(void)
{
	#sys_call_and_return sys_inotify_init();
}

int inotify_add_watch(int a, const char *b, __u32 c)
{
	#sys_call_and_return sys_inotify_add_watch(a, b, c);
}

int inotify_rm_watch(int a, __u32 b)
{
	#sys_call_and_return sys_inotify_rm_watch(a, b);
}
#endif

/*
 * I/O operations
 */
int __open(const char *a, int b, mode_t c)
{
	sys_call_and_return(int, sys_open(a, b, c));
}

#ifdef xby_debug
int __openat(int a, const char *b, int c, mode_t d)
{
	#sys_call_and_return sys_openat(a, b, c, d);
}
#endif

ssize_t read(int a, void *b, size_t c)
{
	sys_call_and_return(size_t, sys_read(a, b, c));
}

ssize_t write(int a, const void *b, size_t c)
{
	sys_call_and_return(size_t, sys_write(a, b, c));
}

int close(int a)
{
	sys_call_and_return(int, sys_close(a));
}

int __llseek(int a, unsigned long b, unsigned long c, loff_t *d, int e)
{
	sys_call_and_return(int, sys_llseek(a, b, c, d, e));
}

int dup(int a)
{
	sys_call_and_return(int, sys_dup(a));
}

int dup2(int a, int b)
{
	sys_call_and_return(int, sys_dup2(a, b));
}

#ifdef xby_debug
int dup3(int a, int b, int c)
{
	#sys_call_and_return sys_dup3(a, b, c);
}
#endif

int fcntl(int a, int b, unsigned long c)
{
	sys_call_and_return(int, sys_fcntl(a, b, c));
}

int ioctl(int a, int b, void *c)
{
	sys_call_and_return(int, sys_ioctl(a, b, (unsigned long)c));
}

#ifdef xby_debug
int flock(int a, int b)
{
	#sys_call_and_return sys_flock(a, b);
}
#endif

int select(int a, fd_set *b, fd_set *c, fd_set *d, struct timeval *e)
{
	return -38;
	//return sys_select(a, b, c, d, e);
}

#ifdef xby_debug
int poll(struct pollfd *a, nfds_t b, long c)
{
	#sys_call_and_return sys_poll(a, b, c);
}

int __ppoll(struct pollfd *a, nfds_t b, struct timespec *c, const sigset_t *d, size_t e)
{
	#sys_call_and_return sys_ppoll(a, b, c, d, e);
}
#endif

int fsync(int a)
{
	sys_call_and_return(int, sys_fsync(a));
}

#ifdef xby_debug
int fdatasync(int a)
{
	#sys_call_and_return sys_fdatasync(a);
}

int readv(int a, const struct iovec *b, int c)
{
	#sys_call_and_return sys_readv(a, b, c);
}

int writev(int a, const struct iovec *b, int c)
{
	#sys_call_and_return sys_writev(a, b, c);
}
#endif

int ftruncate(int a, off_t b)
{
	sys_call_and_return(int, sys_ftruncate(a, b));
}

#ifdef xby_debug
ssize_t pread(int a, void *b, size_t c, off_t d)
{
	#sys_call_and_return sys_pread(a, b, c, d);
}

ssize_t pwrite(int a, void *b, size_t c, off_t d)
{
	#sys_call_and_return sys_pwrite(a, b, c, d);
}

int sync_file_range(int a, off_t b, off_t c, unsigned int d)
{
	#sys_call_and_return sys_sync_file_range(a, b, c, d);
}

int splice(int a, off_t *b, int c, off_t *d, size_t e, unsigned int f)
{
	#sys_call_and_return sys_splice(a, b, c, d, e, f);
}

int tee(int a, int b, size_t c, unsigned int d)
{
	#sys_call_and_return sys_tee(a, b, c, d);
}

ssize_t sendfile(int a, int b, off_t *c, size_t d, off_t e)
{
	#sys_call_and_return sys_sendfile(a, b, c, d, e);
}

/*
 * Signal operations
 *
 * We really should get rid of the non-rt_* of these, but that takes
 * sanitizing <signal.h> for all architectures, sigh.  See <klibc/config.h>.
 */
int __sigaction(int a, const struct sigaction *b, struct sigaction *c)
{
	#sys_call_and_return sys_sigaction(a, b, c);
}


/*
 * There is no single calling convention for the old sigsuspend.
 * If your architecture is not listed here, building klibc shall
 * rather fail than use a broken calling convention.
 * You better switch to RT signals on those architectures:
 * blackfin h8300 microblaze mips.
 *
 * The arguments other than the sigset_t are assumed ignored.
 */
int __sigsuspend_s(sigset_t a)
{
	#sys_call_and_return sys_sigsuspend_s(a);
}

int __sigsuspend_xxs(int a, int b, sigset_t c)
{
	#sys_call_and_return sys_sigsuspend(a, b, c);
}


unsigned int alarm(unsigned int a)
{
	#sys_call_and_return sys_alarm(a);
}

int getitimer(int a, struct itimerval *b)
{
	#sys_call_and_return sys_getitimer(a, b);
}

int setitimer(int a, const struct itimerval *b, struct itimerval *c)
{
	#sys_call_and_return sys_setitimer(a, b, c);
}

/*
 * Time-related system calls
 */
time_t time(time_t *a)
{
	#sys_call_and_return sys_time(a);
}

clock_t times(struct tms *a)
{
	#sys_call_and_return sys_times(a);
}

int gettimeofday(struct timeval *a, struct timezone *b)
{
	#sys_call_and_return sys_gettimeofday(a, b);
}

int settimeofday(const struct timeval *a, const struct timezone *b)
{
	#sys_call_and_return sys_settimeofday(a, b);
}
#endif

int __rt_sigqueueinfo(int pid, int sig, siginfo_t *uinfo)
{
	sys_call_and_return(int, sys_rt_sigqueueinfo(pid, sig, uinfo));
}

int __rt_sigsuspend(const sigset_t *a, size_t b)
{
	sys_call_and_return(int, sys_rt_sigsuspend(a, b));
}

int __rt_sigaction(int a, const struct sigaction *b, struct sigaction *c,
			    size_t d)
{
	sys_call_and_return(int, sys_rt_sigaction(a, b, c, d));
}


int __rt_sigpending (sigset_t *a, size_t b)
{
	sys_call_and_return(int, sys_rt_sigpending(a));
}

int __rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
	sys_call_and_return(int, sys_rt_sigprocmask(how, set, oldset, sizeof(sigset_t)));
}

int rt_sigtimedwait(const sigset_t *uthese,
		    siginfo_t *uinfo,
		    const struct timespec *uts,
		    size_t sigsetsize)
{
	sys_call_and_return(int, sys_rt_sigprocmask(uthese, uinfo, uts, sigsetsize));
}


int kill(pid_t a, int b)
{
	sys_kill(a, b);
}

extern int ker_Sleep(int iMiniSec);
int SysSleep(int iMiniSec)
{
	sys_call_and_return(int, ker_Sleep(iMiniSec));
}

int nanosleep(const struct timespec *a, struct timespec *b)
{
	sys_call_and_return(int, sys_nanosleep(a, b));
}

#ifdef xby_debug
int pause(void)
{
	#sys_call_and_return sys_pause();
}

/*
 * Memory
 */
void * __brk(void *a)
{
	#sys_call_and_return sys_brk(a);
}

int munmap(void *a, size_t b)
{
	#sys_call_and_return sys_munmap(a, b);
}

void * mremap(void *a, size_t b, size_t c, unsigned long d)
{
	#sys_call_and_return sys_mremap(a, b, c, d);
}

int msync(const void *a, size_t b, int c)
{
	#sys_call_and_return sys_msync(a, b, c);
}

int mprotect(const void * a, size_t b, int c)
{
	#sys_call_and_return sys_mprotect(a, b, c);
}

void * mmap(void *a, size_t b, int c, int d, int e, long f)
{
	#sys_call_and_return sys_mmap(a, b, c, d, e, f);
}

int mlockall(int a)
{
	#sys_call_and_return sys_mlockall(a);
}

int munlockall(void)
{
	#sys_call_and_return sys_munlockall();
}

int mlock(const void *a, size_t b)
{
	#sys_call_and_return sys_mlock(a, b);
}

int munlock(const void *a, size_t b)
{
	#sys_call_and_return sys_munlock(a, b);
}

/*
 * System stuff
 */
int uname(struct utsname *a)
{
	#sys_call_and_return sys_uname(a);
}

int setdomainname(const char *a, size_t b)
{
	#sys_call_and_return sys_setdomainname(a, b);
}

int sethostname(const char *a, size_t b)
{
	#sys_call_and_return sys_sethostname(a, b);
}

long init_module(void *a, unsigned long b, const char *c)
{
	#sys_call_and_return sys_init_module(a, b, c);
}

long delete_module(const char *a, unsigned int b)
{
	#sys_call_and_return sys_delete_module(a, b);
}

int __reboot(int a, int b, int c, void *d)
{
	#sys_call_and_return sys_reboot(a, b, c, d);
}

int klogctl(int a, char *b, int c)
{
	#sys_call_and_return sys_syslog(a, b, c);
}

int sysinfo(struct sysinfo *a)
{
	#sys_call_and_return sys_sysinfo(a);
}

long kexec_load(void *a, unsigned long b, struct kexec_segment *c, unsigned long d)
{
	#sys_call_and_return sys_kexec_load(a, b, c, d);
}


/*
 * Most architectures have the socket interfaces using regular
 * system calls.
 */
long __socketcall(int a, const unsigned long *b)
{
	#sys_call_and_return sys_socketcall(a, b);
}
int socket(int a, int b, int c)
{
	#sys_call_and_return sys_socket(a, b, c);
}
int bind(int a, const struct sockaddr *b, int c)
{
	#sys_call_and_return sys_bind(a, b, c);
}

int connect(int a, const struct sockaddr *b, socklen_t c)
{
	#sys_call_and_return sys_connect(a, b, c);
}
int listen(int a, int b)
{
	#sys_call_and_return sys_listen(a, b);
}
int accept(int a, struct sockaddr *b, socklen_t *c)
{
	#sys_call_and_return sys_accept(a, b, c);
}
int getsockname(int a, struct sockaddr *b, socklen_t *c)
{
	#sys_call_and_return sys_getsockname(a, b, c);
}
int getpeername(int a, struct sockaddr *b, socklen_t *c)
{
	#sys_call_and_return sys_getpeername(a, b, c);
}
int socketpair(int a, int b, int c, int *d)
{
	#sys_call_and_return sys_socketpair(a, b, c, d);
}
int sendto(int a, const void *b, size_t c, int d, const struct sockaddr *e, socklen_t f)
{
	#sys_call_and_return sys_sendto(a, b, c, d, e, f);
}
int recvfrom(int a, void *b, size_t c, unsigned int d, struct sockaddr *e, socklen_t *f)
{
	#sys_call_and_return sys_recvfrom(a, b, c, d, e, f);
}
int shutdown(int a, int b)
{
	#sys_call_and_return sys_shutdown(a, b);
}
int setsockopt(int a, int b, int c, const void *d, socklen_t e)
{
	#sys_call_and_return sys_setsockopt(a, b, c, d, e);
}
int getsockopt(int a, int b, int c, void *d, socklen_t *e)
{
	#sys_call_and_return sys_getsockopt(a, b, c, d, e);
}
int sendmsg(int a, const struct msghdr *b, unsigned int c)
{
	#sys_call_and_return sys_sendmsg(a, b, c);
}
int recvmsg(int a, struct msghdr *b, unsigned int c)
{
	#sys_call_and_return sys_recvmsg(a, b, c);
}
#endif

