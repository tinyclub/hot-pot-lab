#include <signal.h>
#include <sys/syscall.h>
#include <klibc/sysconfig.h>

static int
do_sigpause (int sig_or_mask, int is_sig)
{
	sigset_t set;

	if (is_sig != 0)
	{
		/* The modern X/Open implementation is requested.  */
		if (__rt_sigprocmask (0, NULL, &set) < 0
		|| sigdelset (&set, sig_or_mask) < 0)
			return -1;
	}
	else if (sigset_set_old_mask (&set, sig_or_mask) < 0)
		return -1;

	/* Note the sigpause() is a cancellation point.  But since we call
	 sigsuspend() which itself is a cancellation point we do not have
	 to do anything here.  */
	return __rt_sigsuspend (&set);
}

int
__sigpause (int sig_or_mask, int is_sig)
{
	return do_sigpause (sig_or_mask, is_sig);
}

int sigpause (int mask)
{
  return __sigpause (mask, 0);
}

