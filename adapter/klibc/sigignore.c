
#include <errno.h>
#define __need_NULL
#include <stddef.h>
#include <signal.h>
#include <string.h>	/* For the real memset prototype.  */


int
sigignore (int sig)
{
  struct sigaction act;

  act.sa_handler = SIG_IGN;
  if (sigemptyset (&act.sa_mask) < 0)
    return -1;
  act.sa_flags = 0;

  return sigaction (sig, &act, NULL);
}

