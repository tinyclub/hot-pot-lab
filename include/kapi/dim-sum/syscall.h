#ifndef _KAPI_PHOENIX_SYSCALL_H
#define _KAPI_PHOENIX_SYSCALL_H
#include <linux/linkage.h>
#include <linux/utime.h>
#include <linux/time.h>
#include <asm/signal.h>
#include <asm/stat.h>
#include <linux/types.h>
#include <fs/vfs.h>
#include <asm/statfs.h>
#include <linux/resource.h>


extern asmlinkage int sys_setup(void);
asmlinkage __noreturn void sys_exit(int error_code);

#ifdef xby_debug
extern asmlinkage pid_t sys_clone(unsigned long a, void *b);

extern asmlinkage pid_t sys_fork(void);
extern asmlinkage pid_t sys_vfork();
extern asmlinkage pid_t sys_getpid();
extern asmlinkage int sys_setpgid(pid_t a, pid_t b);
extern asmlinkage pid_t sys_getpgid(pid_t a);
extern asmlinkage pid_t sys_getppid(void);

extern asmlinkage pid_t sys_setsid(void);

extern asmlinkage pid_t sys_getsid(pid_t a);
#endif


extern asmlinkage pid_t sys_wait4(pid_t a, int *b, int c, struct rusage *d);

#ifdef xby_debug
extern asmlinkage int sys_execve(const char *a, char * const *b, char * const *c);

extern asmlinkage int sys_nice(int a);
extern asmlinkage int sys_getpriority(int a, int b);

extern asmlinkage int sys_setpriority(int a, int b, int c);

extern asmlinkage int sys_getrusage(int a, struct rusage *b);

extern asmlinkage int sys_sched_setscheduler(pid_t a, int b, const struct sched_param *c);

extern asmlinkage int sys_sched_setaffinity(pid_t a, unsigned int b, unsigned long *c);

extern asmlinkage int sys_sched_getaffinity(pid_t a, unsigned int b, unsigned long *c);

extern asmlinkage int sys_sched_yield(void);

extern asmlinkage int sys_prctl(int a, unsigned long b, unsigned long c, unsigned long d, unsigned long e);

/*
 * User and group IDs
 */
extern asmlinkage int sys_setuid(uid_t a);

extern asmlinkage int sys_setgid(gid_t a);

extern asmlinkage uid_t sys_getuid(void);

extern asmlinkage gid_t sys_getgid(void);

extern asmlinkage uid_t sys_geteuid(void);

extern asmlinkage gid_t sys_getegid(void);

extern asmlinkage int sys_getgroups(int a, gid_t *b);

extern asmlinkage int sys_setgroups(size_t a, const gid_t *b);

extern asmlinkage int sys_setreuid(uid_t a, uid_t b);

extern asmlinkage int sys_setregid(gid_t a, gid_t b);

extern asmlinkage int sys_setfsuid(uid_t a);
extern asmlinkage int sys_setfsgid(gid_t a);

extern asmlinkage int sys_setresuid(int a, uid_t b, uid_t c, uid_t d);

/*
 * POSIX Capabilities
 */
extern asmlinkage int sys_capget(cap_user_header_t a, cap_user_data_t b);
extern asmlinkage int sys_capset(cap_user_header_t a, cap_user_data_t b);
#endif
/*
 * Filesystem-related system calls
 */
extern asmlinkage int sys_mount(char * dev_name, char * dir_name, char * type,
	unsigned long new_flags, const void * data);
extern asmlinkage int sys_umount(const char * name);

extern asmlinkage int sys_umount2(const char *a, int b);

#ifdef xby_debug
extern asmlinkage int sys_pivot_root(const char *a, const char *b);
#endif

extern asmlinkage int sys_sync(void);

extern asmlinkage int sys_statfs(const char *a, struct statfs *b);

extern asmlinkage int sys_fstatfs(unsigned int fd, struct statfs * buf);

#ifdef xby_debug
extern asmlinkage int sys_swapon(const char *a, int b);

extern asmlinkage int sys_swapoff(const char *a);
#endif

/*
 * Inode-related system calls
 */
extern asmlinkage int sys_access(const char *a, int b);

#ifdef xby_debug
extern asmlinkage int sys_faccessat(int a, const char *b, int c, int d);
#endif

extern asmlinkage int sys_link(const char * a, const char *b);

#ifdef xby_debug
extern asmlinkage int sys_linkat(int a, const char *b, int c, const char *d);
#endif

extern asmlinkage int sys_unlink(const char *a);

#ifdef xby_debug
extern asmlinkage int sys_unlinkat(int a, const char *b, int c);
#endif

extern asmlinkage int sys_chdir(const char *a);
extern asmlinkage int sys_fchdir(unsigned int fd);

extern asmlinkage int sys_rename(const char *a, const char *b);
extern asmlinkage int sys_readdir(unsigned int fd, struct dirent * dirent, unsigned int count);

#ifdef xby_debug
extern asmlinkage int sys_renameat(int a, const char *b, int c, const char *d);
#endif

extern asmlinkage int sys_mknod(const char * filename, int mode, dev_t dev);

#ifdef xby_debug
extern asmlinkage int sys_mknodat(int a, const char *b, const char *c, mode_t d, dev_t e);
#endif

extern asmlinkage int sys_chmod(const char *a, mode_t b);
extern asmlinkage int sys_fchmod(unsigned int fd, mode_t mode);

#ifdef xby_debug
extern asmlinkage int sys_fchmodat(int a, const char *b, mode_t c);
#endif

extern asmlinkage int sys_mkdir(const char * pathname, int mode);

#ifdef xby_debug
extern asmlinkage int sys_mkdirat(int a, const char *b, const char *c, mode_t d);
#endif

extern asmlinkage int sys_rmdir(const char *a);

extern asmlinkage int sys_pipe(int *a);

#ifdef xby_debug
extern asmlinkage int sys_pipe2(int *a, int b);
#endif

extern asmlinkage mode_t sys_umask(mode_t a);


extern asmlinkage int sys_chroot(const char *a);

extern asmlinkage int sys_symlink(const char *a, const char *b);

#ifdef xby_debug
extern asmlinkage int sys_symlinkat(const char *a, int b, const char *c);
#endif

extern asmlinkage int sys_readlink(const char * path, char * buf, int bufsiz);

#ifdef xby_debug
extern asmlinkage int sys_readlinkat(int a, const char *b, char *c, int d);
#endif

extern asmlinkage int sys_stat(const char * filename, struct stat * statbuf);

extern asmlinkage int sys_lstat(const char * filename, struct stat * statbuf);

extern asmlinkage int sys_fstat(unsigned int fd, struct stat * statbuf);

/* XXX: Is this right?! */
#ifdef xby_debug
extern asmlinkage int sys_fstatat(int a, const char *b, struct stat *c, int d);
#endif

//extern asmlinkage int sys_getdents(unsigned int a, struct dirent *b, unsigned int c);


extern asmlinkage int sys_chown(const char *a, uid_t b, gid_t c);
extern asmlinkage int sys_fchown(unsigned int fd, uid_t user, gid_t group);

#ifdef xby_debug
extern asmlinkage int sys_fchownat(int a, const char *b, uid_t c, gid_t d, int e);

extern asmlinkage int sys_lchown(const char * a, uid_t b, gid_t c);
#endif

extern asmlinkage int sys_getcwd(char *a, size_t b);

extern asmlinkage int sys_utime(const char * filename, const struct utimbuf * times);

#ifdef xby_debug
extern asmlinkage int sys_utimes(const char *a, const struct timeval *b);

extern asmlinkage int sys_futimesat(int a, const char *b, const struct timeval *c);

extern asmlinkage int sys_inotify_init(void);

extern asmlinkage int sys_inotify_add_watch(int a, const char *b, __u32 c);

extern asmlinkage int sys_inotify_rm_watch(int a, __u32 b);
#endif

/*
 * I/O operations
 */
extern asmlinkage int sys_open(const char * filename,int flags,int mode);

#ifdef xby_debug
extern asmlinkage int sys_openat(int a, const char *b, int c, mode_t d);
#endif

extern asmlinkage ssize_t sys_read(unsigned int fd,char * buf,unsigned int count);

extern asmlinkage ssize_t sys_write(unsigned int fd,const char * buf,unsigned int count);

extern asmlinkage int sys_close(unsigned int fd);

extern asmlinkage int sys_llseek(unsigned int fd, unsigned long offset_high,
			  unsigned long offset_low, loff_t * result,
			  unsigned int origin);
extern asmlinkage int sys_dup(unsigned int fildes);
extern asmlinkage int sys_dup2(unsigned int oldfd, unsigned int newfd);

#ifdef xby_debug
extern asmlinkage int sys_dup3(int a, int b, int c);
#endif

extern asmlinkage int sys_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg);

extern asmlinkage int sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);

#ifdef xby_debug
extern asmlinkage int sys_flock(int a, int b);
#endif

extern asmlinkage int sys_select( unsigned long *buffer );

#ifdef xby_debug
extern asmlinkage int sys_poll(struct pollfd *a, nfds_t b, long c);

extern asmlinkage int sys_ppoll(struct pollfd *a, nfds_t b, struct timespec *c, const sigset_t *d, size_t e);
#endif

extern asmlinkage int sys_fsync(unsigned int a);

#ifdef xby_debug
extern asmlinkage int sys_fdatasync(int a);

extern asmlinkage int sys_readv(int a, const struct iovec *b, int c);

extern asmlinkage int sys_writev(int a, const struct iovec *b, int c);
#endif

extern asmlinkage int sys_ftruncate(unsigned int fd, unsigned int length);

#ifdef xby_debug
extern asmlinkage ssize_t sys_pread(int a, void *b, size_t c, off_t d);

extern asmlinkage ssize_t sys_pwrite(int a, void *b, size_t c, off_t d);

extern asmlinkage int sys_sync_file_range(int a, off_t b, off_t c, unsigned int d);

extern asmlinkage int sys_splice(int a, off_t *b, int c, off_t *d, size_t e, unsigned int f);
extern asmlinkage int sys_tee(int a, int b, size_t c, unsigned int d);
extern asmlinkage ssize_t sys_sendfile(int a, int b, off_t *c, size_t d, off_t e);

/*
 * Signal operations
 *
 * We really should get rid of the non-rt_* of these, but that takes
 * sanitizing <signal.h> for all architectures, sigh.  See <klibc/config.h>.
 */
extern asmlinkage int sys_sigaction(int a, const struct sigaction *b, struct sigaction *c);
extern asmlinkage int sys_sigpending(sigset_t *a);
extern asmlinkage int sys_sigprocmask(int a, const sigset_t *b, sigset_t *c);

/*
 * There is no single calling convention for the old sigsuspend.
 * If your architecture is not listed here, building klibc shall
 * rather fail than use a broken calling convention.
 * You better switch to RT signals on those architectures:
 * blackfin h8300 microblaze mips.
 *
 * The arguments other than the sigset_t are assumed ignored.
 */
extern asmlinkage int sys_sigsuspend_s(sigset_t a);

extern asmlinkage int sys_sigsuspend_xxs(int a, int b, sigset_t c);

extern asmlinkage int sys_kill(pid_t a, int b);

extern asmlinkage unsigned int sys_alarm(unsigned int a);

extern asmlinkage int sys_getitimer(int a, struct itimerval *b);

extern asmlinkage int sys_setitimer(int a, const struct itimerval *b, struct itimerval *c);
/*
 * Time-related system calls
 */
extern asmlinkage time_t sys_time(time_t *a);

extern asmlinkage clock_t sys_times(struct tms *a);

extern asmlinkage int sys_gettimeofday(struct timeval *a, struct timezone *b);

extern asmlinkage int sys_settimeofday(const struct timeval *a, const struct timezone *b);
#endif


extern asmlinkage int sys_nanosleep(const struct timespec *a, struct timespec *b);

extern asmlinkage int sys_rt_sigsuspend(const sigset_t *, size_t);
extern asmlinkage long sys_rt_sigaction(int sig,
		 const struct sigaction *act,
		 struct sigaction *oact,
		 size_t sigsetsize);

#ifdef xby_debug

extern asmlinkage int sys_pause(void);

/*
 * Memory
 */
extern asmlinkage void * sys_brk(void *a);

extern asmlinkage int sys_munmap(void *a, size_t b);

extern asmlinkage void * sys_mremap(void *a, size_t b, size_t c, unsigned long d);

extern asmlinkage int sys_msync(const void *a, size_t b, int c);

extern asmlinkage int sys_mprotect(const void * a, size_t b, int c);

extern asmlinkage void * sys_mmap(void *a, size_t b, int c, int d, int e, long f);

extern asmlinkage int sys_mlockall(int a);

extern asmlinkage int sys_munlockall(void);

extern asmlinkage int sys_mlock(const void *a, size_t b);

extern asmlinkage int sys_munlock(const void *a, size_t b);

/*
 * System stuff
 */
extern asmlinkage int sys_uname(struct utsname *a);

extern asmlinkage int sys_setdomainname(const char *a, size_t b);

extern asmlinkage int sys_sethostname(const char *a, size_t b);

extern asmlinkage long sys_init_module(void *a, unsigned long b, const char *c);

extern asmlinkage long sys_delete_module(const char *a, unsigned int b);

extern asmlinkage int sys_reboot(int a, int b, int c, void *d);

extern asmlinkage int sys_klogctl(int a, char *b, int c);

extern asmlinkage int sys_sysinfo(struct sysinfo *a);

extern asmlinkage long sys_kexec_load(void *a, unsigned long b, struct kexec_segment *c, unsigned long d);

/*
 * Most architectures have the socket interfaces using regular
 * system calls.
 */
extern asmlinkage long sys_socketcall(int a, const unsigned long *b);
extern asmlinkage int sys_socket(int a, int b, int c);
extern asmlinkage int sys_bind(int a, const struct sockaddr *b, int c);

extern asmlinkage int sys_connect(int a, const struct sockaddr *b, socklen_t c);
extern asmlinkage int sys_listen(int a, int b);
extern asmlinkage int sys_accept(int a, struct sockaddr *b, socklen_t *c);
extern asmlinkage int sys_getsockname(int a, struct sockaddr *b, socklen_t *c);
extern asmlinkage int sys_getpeername(int a, struct sockaddr *b, socklen_t *c);
extern asmlinkage int sys_socketpair(int a, int b, int c, int *d);
extern asmlinkage int sys_sendto(int a, const void *b, size_t c, int d, const struct sockaddr *e, socklen_t f);
extern asmlinkage int sys_recvfrom(int a, void *b, size_t c, unsigned int d, struct sockaddr *e, socklen_t *f);
extern asmlinkage int sys_shutdown(int a, int b);
extern asmlinkage int sys_setsockopt(int a, int b, int c, const void *d, socklen_t e);
extern asmlinkage int sys_getsockopt(int a, int b, int c, void *d, socklen_t *e);
extern asmlinkage int sys_sendmsg(int a, const struct msghdr *b, unsigned int c);
extern asmlinkage int sys_recvmsg(int a, struct msghdr *b, unsigned int c);
#endif

#endif

