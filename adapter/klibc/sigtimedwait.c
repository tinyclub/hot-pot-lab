/*
 * sigsuspend.c
 */

#include <signal.h>
#include <sys/syscall.h>
#include <klibc/sysconfig.h>
#include <asm-generic/siginfo.h>
#include <linux/time.h>
//#include <klibc/havesyscall.h>

static int
do_sigtimedwait (const sigset_t *set, siginfo_t *info,
		 const struct timespec *timeout)
{
  	int result = rt_sigtimedwait(set,
			       info, timeout, _NSIG / 8);

	/* The kernel generates a SI_TKILL code in si_code in case tkill is
       used.  tkill is transparently used in raise().  Since having
       SI_TKILL as a code is useful in general we fold the results
       here.  */
	if (result != -1 && info != NULL && info->si_code == SI_TKILL)
		info->si_code = SI_USER;

  return result;
}


/* Return any pending signal or wait for one for the given time.  */
int
__sigtimedwait (const sigset_t *set, siginfo_t *info, const struct timespec *timeout)
{
	return do_sigtimedwait(set, info, timeout);
}

