/*
 * signal.h
 */

#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <klibc/compiler.h>
#include <klibc/extern.h>
#include <string.h>		/* For memset() */
#include <limits.h>		/* For LONG_BIT */
#include <sys/types.h>

#include <klibc/archsignal.h>	/* Includes <asm/signal.h> if appropriate */

#include <asm-generic/siginfo.h>
#include <linux/time.h>
/* glibc seems to use sig_atomic_t as "int" pretty much on all architectures.
   Do the same, but allow the architecture to override. */
#ifndef _KLIBC_HAS_ARCH_SIG_ATOMIC_T
typedef int sig_atomic_t;
#endif

/* Some architectures don't define these */
#ifndef SA_RESETHAND
# define SA_RESETHAND SA_ONESHOT
#endif
#ifndef SA_NODEFER
# define SA_NODEFER SA_NOMASK
#endif
/* Some architectures define NSIG and not _NSIG or vice versa */
#ifndef NSIG
# define NSIG _NSIG
#endif
#ifndef _NSIG
# define _NSIG NSIG
#endif

/* If we don't have any real-time signals available to userspace,
   hide them all */
#if SIGRTMAX <= SIGRTMIN
# undef SIGRTMIN
# undef SIGRTMAX
#endif

/* The kernel header files are inconsistent whether or not
   SIGRTMAX is inclusive or exclusive.  POSIX seems to state that
   it's inclusive, however. */
#if SIGRTMAX >= _NSIG
# undef  SIGRTMAX
# define SIGRTMAX (_NSIG-1)
#endif

__extern const char *const sys_siglist[_NSIG];
__extern const char *const sys_sigabbrev[_NSIG];

/* This assumes sigset_t is either an unsigned long or an array of such,
   and that _NSIG_BPW in the kernel is always LONG_BIT */

static __inline__ int sigemptyset(sigset_t * __set)
{
	memset(__set, 0, sizeof *__set);
	return 0;
}
static __inline__ int sigfillset(sigset_t * __set)
{
	memset(__set, ~0, sizeof *__set);
	return 0;
}
static __inline__ int sigaddset(sigset_t * __set, int __signum)
{
	unsigned long *__lset = (unsigned long *)__set;
	__signum--;		/* Signal 0 is not in the set */
	__lset[__signum / LONG_BIT] |= 1UL << (__signum % LONG_BIT);
	return 0;
}
static __inline__ int sigdelset(sigset_t * __set, int __signum)
{
	unsigned long *__lset = (unsigned long *)__set;
	__signum--;		/* Signal 0 is not in the set */
	__lset[__signum / LONG_BIT] &= ~(1UL << (__signum % LONG_BIT));
	return 0;
}
static __inline__ int sigismember(sigset_t * __set, int __signum)
{
	unsigned long *__lset = (unsigned long *)__set;
	__signum--;		/* Signal 0 is not in the set */
	return (int)((__lset[__signum / LONG_BIT] >> (__signum % LONG_BIT)) &
		     1);
}
static inline int __attribute__ ((unused)) sigset_set_old_mask (sigset_t *set, int mask)
{
  unsigned long int *ptr;
  int cnt;

  ptr = &set->sig[0];

  *ptr++ = (unsigned int) mask;

  cnt = _NSIG_WORDS - 2;
  do
    *ptr++ = 0ul;
  while (--cnt >= 0);

  return 0;
}

static inline int __attribute__ ((unused)) sigset_get_old_mask (const sigset_t *set)
{
  return (unsigned int) set->sig[0];
}

__extern __sighandler_t __signal(int, __sighandler_t, int);
#ifndef signal
__extern __sighandler_t signal(int, __sighandler_t);
#endif
__extern __sighandler_t sysv_signal(int, __sighandler_t);
__extern __sighandler_t bsd_signal(int, __sighandler_t);
__extern int sigaction(int, const struct sigaction *, struct sigaction *);
__extern int sigprocmask(int, const sigset_t *, sigset_t *);
__extern int sigpending(sigset_t *);
__extern int sigsuspend(const sigset_t *);
__extern int sigqueue(pid_t pid, int sig, const union sigval value);
__extern int sigsuspend(const sigset_t *mask);
__extern int sigwait(const sigset_t *set, int *sig);
__extern int sigtimedwait(const sigset_t *set, siginfo_t *info,
				 const struct timespec *timeout);
__extern int sigwaitinfo(const sigset_t *set, siginfo_t *info);
__extern int sighold(int sig);
__extern int sigignore (int sig);
__extern int sigpause(int sig);
__extern int sigrelse(int sig);

__extern int raise(int);
__extern int kill(pid_t, int);

#endif				/* _SIGNAL_H */
