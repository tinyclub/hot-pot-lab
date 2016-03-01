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
do_sigwaitinfo (const sigset_t *set, siginfo_t *info)
{
	/* XXX The size argument hopefully will have to be changed to the
	   real size of the user-level sigset_t.  */
	int result = rt_sigtimedwait(set,
			       info, NULL, _NSIG / 8);

	/* The kernel generates a SI_TKILL code in si_code in case tkill is
	   used.  tkill is transparently used in raise().  Since having
	   SI_TKILL as a code is useful in general we fold the results
	   here.  */
	if (result != -1 && info != NULL && info->si_code == SI_TKILL) {
		info->si_code = SI_USER;
	}

	return result;
}


/* Return any pending signal or wait for one for the given time.  */
int
__sigwaitinfo (const sigset_t *set, siginfo_t *info)
{
	return do_sigwaitinfo (set, info);
}

