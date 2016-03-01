#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <asm-generic/siginfo.h>

//#include <sysdep.h>

extern int __rt_sigqueueinfo(int pid, int sig, siginfo_t *uinfo);
int
__sigqueue (pid_t pid, int sig, const union sigval val)
{
  siginfo_t info;

  /* First, clear the siginfo_t structure, so that we don't pass our
     stack content to other tasks.  */
  memset (&info, 0, sizeof (siginfo_t));
  /* We must pass the information about the data in a siginfo_t value.  */
  info.si_signo = sig;
  info.si_code = SI_QUEUE;
  info.si_pid = getpid();
  info.si_uid = getuid();
  info.si_value = val;

  return __rt_sigqueueinfo(pid, sig, &info);
}

