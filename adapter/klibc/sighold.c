
#define __need_NULL
#include <stddef.h>
#include <signal.h>

int
sighold (sig)
     int sig;
{
  sigset_t set;

  /* Retrieve current signal set.  */
  if (__rt_sigprocmask (SIG_SETMASK, NULL, &set) < 0)
    return -1;

  /* Add the specified signal.  */
  if (sigaddset (&set, sig) < 0)
    return -1;

  /* Set the new mask.  */
  return __rt_sigprocmask (SIG_SETMASK, &set, NULL);
}

