/*
 * sigsuspend.c
 */

#include <signal.h>
#include <sys/syscall.h>
#include <klibc/sysconfig.h>
//#include <klibc/havesyscall.h>

/* Return any pending signal or wait for one for the given time.  */
static int
do_sigwait (const sigset_t *set, int *sig)
{
	int ret;


	do {
		ret = rt_sigtimedwait(set,
			  NULL, NULL, _NSIG / 8);
	} while (ret == -1 && errno == EINTR);

	if (ret != -1) {
		*sig = ret;
		ret = 0;
	}
	else {
		ret = errno;
	}
	
	return ret;
}

int
__sigwait (const sigset_t *set, int *sig)
{
  return do_sigwait (set, sig);
}

